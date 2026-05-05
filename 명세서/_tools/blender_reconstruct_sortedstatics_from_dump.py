"""
Reconstruct missing SortedStatics mesh objects in Blender using the Unity dump.

Goal
- The imported FBX already contains the scene anchors and many correct meshes.
- Some families are imported only as placeholder empties/nulls with no real mesh.
- Use the dump as the source of truth for which GameObject instance should use
  which mesh reference, then borrow mesh data from any already-imported Blender
  object that shares the same mesh reference.

What this script does
1. Parse dump GameObject / Transform / MeshFilter.
2. Infer a dump->Blender affine map from unique anchor names.
3. Match dump mesh-bearing instances to current Blender objects by name and position.
4. Build a template library: mesh_ref -> existing Blender mesh object.
5. For Blender placeholder objects with matching dump mesh refs but no mesh data,
   create a replacement mesh object using the shared template geometry.

Result
- This does not rewrite the binary FBX on disk.
- It reconstructs a viewable complete scene inside Blender.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from collections import Counter, defaultdict
import math
import re
import struct


DUMP_DIR = Path(
    r"C:\Users\chaho\ddoichaboom\Solo_Leveling\Resources\Models\map\Chap005\ThroneRoom\ThroneRoom_Split\dump"
)

TARGET_COLLECTION_NAME = None

MIN_ANCHORS = 8
MATCH_DISTANCE = 0.2
HIDE_PLACEHOLDERS = True
RENAME_PLACEHOLDERS = True
CREATE_REPLACEMENT_SUFFIX = "__REC"
VERBOSE = True


BLENDER_DUP_SUFFIX_RE = re.compile(r"\.\d{3}$")


@dataclass
class DumpTransform:
    transform_id: int
    local_position: tuple[float, float, float]
    local_rotation_xyzw: tuple[float, float, float, float]
    local_scale: tuple[float, float, float]
    father_transform_id: int | None
    world_matrix: list[list[float]] | None = None


@dataclass
class DumpInstance:
    gameobject_path_id: int
    gameobject_name: str
    transform_id: int
    mesh_ref: tuple[int, int]
    world_position: tuple[float, float, float]
    parent_name: str | None


def log(message: str) -> None:
    if VERBOSE:
        print(message)


def normalize_name(name: str) -> str:
    return BLENDER_DUP_SUFFIX_RE.sub("", name).strip()


def mat_mul(left: list[list[float]], right: list[list[float]]) -> list[list[float]]:
    out = [[0.0] * 4 for _ in range(4)]
    for row in range(4):
        for col in range(4):
            out[row][col] = sum(left[row][k] * right[k][col] for k in range(4))
    return out


def quat_to_matrix_xyzw(x: float, y: float, z: float, w: float) -> list[list[float]]:
    xx = x * x
    yy = y * y
    zz = z * z
    xy = x * y
    xz = x * z
    yz = y * z
    wx = w * x
    wy = w * y
    wz = w * z
    return [
        [1.0 - 2.0 * (yy + zz), 2.0 * (xy - wz), 2.0 * (xz + wy), 0.0],
        [2.0 * (xy + wz), 1.0 - 2.0 * (xx + zz), 2.0 * (yz - wx), 0.0],
        [2.0 * (xz - wy), 2.0 * (yz + wx), 1.0 - 2.0 * (xx + yy), 0.0],
        [0.0, 0.0, 0.0, 1.0],
    ]


def trs_matrix(
    position: tuple[float, float, float],
    rotation_xyzw: tuple[float, float, float, float],
    scale: tuple[float, float, float],
) -> list[list[float]]:
    px, py, pz = position
    sx, sy, sz = scale
    x, y, z, w = rotation_xyzw

    scale_mat = [
        [sx, 0.0, 0.0, 0.0],
        [0.0, sy, 0.0, 0.0],
        [0.0, 0.0, sz, 0.0],
        [0.0, 0.0, 0.0, 1.0],
    ]
    rot_mat = quat_to_matrix_xyzw(x, y, z, w)
    trans_mat = [
        [1.0, 0.0, 0.0, px],
        [0.0, 1.0, 0.0, py],
        [0.0, 0.0, 1.0, pz],
        [0.0, 0.0, 0.0, 1.0],
    ]
    return mat_mul(trans_mat, mat_mul(rot_mat, scale_mat))


def parse_gameobject_file(path: Path) -> tuple[int, str, list[int]]:
    data = path.read_bytes()
    component_count = struct.unpack_from("<i", data, 0)[0]
    offset = 4
    components = []
    for _ in range(component_count):
        components.append(struct.unpack_from("<q", data, offset + 4)[0])
        offset += 12
    offset += 4
    name_len = struct.unpack_from("<i", data, offset)[0]
    offset += 4
    name = data[offset : offset + name_len].decode("utf-8", errors="replace")

    if "#" in path.stem:
        gameobject_path_id = int(path.stem.split("#", 1)[1].strip())
    else:
        gameobject_path_id = hash(path.name) & 0x7FFFFFFF
    return gameobject_path_id, name, components


def parse_transform_file(path: Path) -> tuple[int, int, DumpTransform]:
    data = path.read_bytes()
    transform_id = int(path.stem.split("#", 1)[1].strip())
    gameobject_path_id = struct.unpack_from("<q", data, 4)[0]
    rotation_xyzw = struct.unpack_from("<4f", data, 12)
    position = struct.unpack_from("<3f", data, 28)
    scale = struct.unpack_from("<3f", data, 40)

    child_count = struct.unpack_from("<i", data, 52)[0]
    offset = 56 + child_count * 12
    father_transform_id = None
    if offset + 12 <= len(data):
        father_transform_id = struct.unpack_from("<q", data, offset + 4)[0]

    return transform_id, gameobject_path_id, DumpTransform(
        transform_id=transform_id,
        local_position=position,
        local_rotation_xyzw=rotation_xyzw,
        local_scale=scale,
        father_transform_id=father_transform_id,
    )


def parse_meshfilter_file(path: Path) -> tuple[int, int, tuple[int, int]]:
    data = path.read_bytes()
    meshfilter_id = int(path.stem.split("#", 1)[1].strip())
    gameobject_path_id = struct.unpack_from("<q", data, 4)[0]
    mesh_file_id = struct.unpack_from("<i", data, 12)[0]
    mesh_path_id = struct.unpack_from("<q", data, 16)[0]
    return meshfilter_id, gameobject_path_id, (mesh_file_id, mesh_path_id)


def load_dump_instances(dump_dir: Path) -> list[DumpInstance]:
    gameobjects_by_goid = {}
    components_by_goid = {}
    component_type_by_id = {}

    for subdir in dump_dir.iterdir():
        if not subdir.is_dir():
            continue
        if subdir.name == "GameObject":
            continue
        for path in subdir.glob("*#*.dat"):
            try:
                component_id = int(path.stem.split("#", 1)[1].strip())
            except Exception:
                continue
            component_type_by_id[component_id] = subdir.name

    for path in (dump_dir / "GameObject").glob("*.dat"):
        goid, name, components = parse_gameobject_file(path)
        gameobjects_by_goid[goid] = name
        components_by_goid[goid] = components

    transforms_by_id = {}
    transform_by_goid = {}
    for path in (dump_dir / "Transform").glob("*.dat"):
        transform_id, goid, transform = parse_transform_file(path)
        transforms_by_id[transform_id] = transform
        transform_by_goid[goid] = transform_id

    mesh_ref_by_goid = {}
    for path in (dump_dir / "MeshFilter").glob("*.dat"):
        _, goid, mesh_ref = parse_meshfilter_file(path)
        mesh_ref_by_goid[goid] = mesh_ref

    transform_name_by_id = {}
    for goid, name in gameobjects_by_goid.items():
        transform_id = transform_by_goid.get(goid)
        if transform_id is not None:
            transform_name_by_id[transform_id] = name

    def compute_world(transform_id: int) -> list[list[float]]:
        transform = transforms_by_id[transform_id]
        if transform.world_matrix is not None:
            return transform.world_matrix
        local = trs_matrix(
            transform.local_position,
            transform.local_rotation_xyzw,
            transform.local_scale,
        )
        father = transform.father_transform_id
        if father is None or father == transform_id or father not in transforms_by_id:
            world = local
        else:
            world = mat_mul(compute_world(father), local)
        transform.world_matrix = world
        return world

    instances = []
    for goid, mesh_ref in mesh_ref_by_goid.items():
        transform_id = transform_by_goid.get(goid)
        if transform_id is None:
            continue
        transform = transforms_by_id[transform_id]
        world = compute_world(transform_id)
        father = transform.father_transform_id
        parent_name = transform_name_by_id.get(father) if father in transform_name_by_id else None
        instances.append(
            DumpInstance(
                gameobject_path_id=goid,
                gameobject_name=gameobjects_by_goid.get(goid, f"GameObject_{goid}"),
                transform_id=transform_id,
                mesh_ref=mesh_ref,
                world_position=(world[0][3], world[1][3], world[2][3]),
                parent_name=parent_name,
            )
        )
    return instances


def solve_linear_system_4x4(matrix4: list[list[float]], vector4: list[float]) -> list[float]:
    aug = [row[:] + [vector4[index]] for index, row in enumerate(matrix4)]
    size = 4
    for pivot in range(size):
        best_row = max(range(pivot, size), key=lambda row: abs(aug[row][pivot]))
        if abs(aug[best_row][pivot]) < 1e-8:
            raise ValueError("Singular matrix while solving affine transform.")
        if best_row != pivot:
            aug[pivot], aug[best_row] = aug[best_row], aug[pivot]
        pivot_value = aug[pivot][pivot]
        for col in range(pivot, size + 1):
            aug[pivot][col] /= pivot_value
        for row in range(size):
            if row == pivot:
                continue
            factor = aug[row][pivot]
            if factor == 0.0:
                continue
            for col in range(pivot, size + 1):
                aug[row][col] -= factor * aug[pivot][col]
    return [aug[row][size] for row in range(size)]


def fit_affine_from_points(
    source_points: list[tuple[float, float, float]],
    target_points: list[tuple[float, float, float]],
) -> list[list[float]]:
    ata = [[0.0] * 4 for _ in range(4)]
    atb_x = [0.0] * 4
    atb_y = [0.0] * 4
    atb_z = [0.0] * 4

    for (sx, sy, sz), (tx, ty, tz) in zip(source_points, target_points):
        row = [sx, sy, sz, 1.0]
        for i in range(4):
            for j in range(4):
                ata[i][j] += row[i] * row[j]
            atb_x[i] += row[i] * tx
            atb_y[i] += row[i] * ty
            atb_z[i] += row[i] * tz

    coeff_x = solve_linear_system_4x4(ata, atb_x)
    coeff_y = solve_linear_system_4x4(ata, atb_y)
    coeff_z = solve_linear_system_4x4(ata, atb_z)

    return [
        [coeff_x[0], coeff_x[1], coeff_x[2], coeff_x[3]],
        [coeff_y[0], coeff_y[1], coeff_y[2], coeff_y[3]],
        [coeff_z[0], coeff_z[1], coeff_z[2], coeff_z[3]],
        [0.0, 0.0, 0.0, 1.0],
    ]


def transform_point_affine(matrix4: list[list[float]], point3: tuple[float, float, float]) -> tuple[float, float, float]:
    x, y, z = point3
    return (
        matrix4[0][0] * x + matrix4[0][1] * y + matrix4[0][2] * z + matrix4[0][3],
        matrix4[1][0] * x + matrix4[1][1] * y + matrix4[1][2] * z + matrix4[1][3],
        matrix4[2][0] * x + matrix4[2][1] * y + matrix4[2][2] * z + matrix4[2][3],
    )


def distance(left: tuple[float, float, float], right: tuple[float, float, float]) -> float:
    return math.dist(left, right)


def find_best_instance(
    candidates: list[DumpInstance],
    affine: list[list[float]],
    blender_position: tuple[float, float, float],
    parent_name: str | None,
    used_transform_ids: set[int],
) -> tuple[DumpInstance | None, float]:
    best = None
    best_distance = float("inf")
    for instance in candidates:
        if instance.transform_id in used_transform_ids:
            continue
        if parent_name is not None and instance.parent_name is not None and parent_name != instance.parent_name:
            continue
        mapped = transform_point_affine(affine, instance.world_position)
        current_distance = distance(mapped, blender_position)
        if current_distance < best_distance:
            best_distance = current_distance
            best = instance
    return best, best_distance


def run_in_blender() -> None:
    try:
        import bpy
    except ImportError as exc:
        raise RuntimeError("This script must run inside Blender.") from exc

    dump_instances = load_dump_instances(DUMP_DIR)
    dump_by_name = defaultdict(list)
    for instance in dump_instances:
        dump_by_name[instance.gameobject_name].append(instance)

    if TARGET_COLLECTION_NAME is None:
        blender_objects = list(bpy.data.objects)
    else:
        collection = bpy.data.collections.get(TARGET_COLLECTION_NAME)
        if collection is None:
            raise KeyError(f"Collection not found: {TARGET_COLLECTION_NAME}")
        blender_objects = list(collection.all_objects)

    blender_by_base = defaultdict(list)
    for obj in blender_objects:
        blender_by_base[normalize_name(obj.name)].append(obj)

    anchor_pairs = []
    for name, instances in dump_by_name.items():
        if len(instances) != 1:
            continue
        objects = blender_by_base.get(name, [])
        if len(objects) != 1:
            continue
        anchor_pairs.append((instances[0].world_position, tuple(objects[0].matrix_world.to_translation()), name))

    log(f"[Reconstruct] dump_instances={len(dump_instances)} blender_objects={len(blender_objects)}")
    log(f"[Reconstruct] unique_anchor_candidates={len(anchor_pairs)}")

    if len(anchor_pairs) < MIN_ANCHORS:
        raise RuntimeError(
            f"Not enough anchors to infer dump->Blender transform. "
            f"Found {len(anchor_pairs)}, need at least {MIN_ANCHORS}."
        )

    affine = fit_affine_from_points(
        [pair[0] for pair in anchor_pairs],
        [pair[1] for pair in anchor_pairs],
    )

    # Match existing Blender mesh objects to dump instances and build template library.
    used_dump_transforms = set()
    template_by_mesh_ref = {}
    matched_mesh_objects = 0

    for base_name, objects in blender_by_base.items():
        candidates = dump_by_name.get(base_name, [])
        if not candidates:
            continue

        mesh_objects = [obj for obj in objects if obj.type == "MESH"]
        for obj in mesh_objects:
            parent_name = normalize_name(obj.parent.name) if obj.parent else None
            instance, best_distance = find_best_instance(
                candidates,
                affine,
                tuple(obj.matrix_world.to_translation()),
                parent_name,
                used_dump_transforms,
            )
            if instance is None or best_distance > MATCH_DISTANCE:
                continue
            used_dump_transforms.add(instance.transform_id)
            template_by_mesh_ref.setdefault(instance.mesh_ref, obj)
            matched_mesh_objects += 1

    log(f"[Reconstruct] template_mesh_refs={len(template_by_mesh_ref)} matched_mesh_objects={matched_mesh_objects}")

    # Find placeholder objects and reconstruct missing meshes.
    reconstructed = 0
    unresolved = 0
    skipped_existing_mesh = 0
    used_placeholder_transforms = set()

    for base_name, objects in blender_by_base.items():
        candidates = dump_by_name.get(base_name, [])
        if not candidates:
            continue

        for obj in objects:
            if obj.type == "MESH":
                continue

            if any(child.type == "MESH" for child in obj.children):
                skipped_existing_mesh += 1
                continue

            parent_name = normalize_name(obj.parent.name) if obj.parent else None
            instance, best_distance = find_best_instance(
                candidates,
                affine,
                tuple(obj.matrix_world.to_translation()),
                parent_name,
                used_placeholder_transforms,
            )
            if instance is None or best_distance > MATCH_DISTANCE:
                continue

            used_placeholder_transforms.add(instance.transform_id)
            template = template_by_mesh_ref.get(instance.mesh_ref)
            if template is None:
                unresolved += 1
                log(
                    f"[Reconstruct] unresolved placeholder={obj.name} "
                    f"mesh_ref={instance.mesh_ref} parent={parent_name}"
                )
                continue

            new_name = obj.name + CREATE_REPLACEMENT_SUFFIX
            new_obj = bpy.data.objects.new(new_name, template.data)
            new_obj.matrix_world = obj.matrix_world.copy()
            if obj.parent is not None:
                new_obj.parent = obj.parent
                new_obj.matrix_parent_inverse = obj.matrix_parent_inverse.copy()
                new_obj.location = obj.location.copy()
                new_obj.rotation_mode = obj.rotation_mode
                if obj.rotation_mode == "QUATERNION":
                    new_obj.rotation_quaternion = obj.rotation_quaternion.copy()
                elif obj.rotation_mode == "AXIS_ANGLE":
                    new_obj.rotation_axis_angle = tuple(obj.rotation_axis_angle)
                else:
                    new_obj.rotation_euler = obj.rotation_euler.copy()
                new_obj.scale = obj.scale.copy()

            if TARGET_COLLECTION_NAME is None:
                bpy.context.scene.collection.objects.link(new_obj)
            else:
                bpy.data.collections[TARGET_COLLECTION_NAME].objects.link(new_obj)

            if RENAME_PLACEHOLDERS:
                obj.name = obj.name + "__EMPTY"
            if HIDE_PLACEHOLDERS:
                obj.hide_set(True)
                obj.hide_render = True

            reconstructed += 1
            log(
                f"[Reconstruct] created mesh={new_obj.name} "
                f"from template={template.name} placeholder={obj.name} "
                f"mesh_ref={instance.mesh_ref}"
            )

    log(
        f"[Reconstruct] reconstructed={reconstructed} "
        f"unresolved={unresolved} "
        f"skipped_existing_mesh={skipped_existing_mesh}"
    )


if __name__ == "__main__":
    run_in_blender()
