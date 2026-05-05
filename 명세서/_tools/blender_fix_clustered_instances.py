"""
Fix clustered repeated instances in Blender using Unity AssetStudio raw dump data.

Use this when:
- The imported FBX already looks mostly like the map.
- Some repeated meshes are piled up in one place.
- You do NOT want to reapply transforms to the whole scene.

Current ThroneRoom setup
- The relevant Unity root is Transform pathID 4668.
- That root is identity TRS and has 2414 direct children.
- So this script should use the explicit root transform pathID instead of trying
  to rediscover the root by name alone.

Workflow
1. Import the extracted FBX into Blender.
2. Open this script in Blender's Text Editor.
3. Adjust the config block if needed.
4. Select one problematic family first if ONLY_SELECTED_FAMILIES is enabled.
5. Run the script.

Important
- This script prefers exact matching through both object name and mesh data name.
- It also has a canonical-name fallback for cases where FBX object names differ
  from Unity GameObject names.
- By default it moves only "repeated and clustered" groups, not unique objects.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from collections import defaultdict, Counter
import math
import re
import struct


# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

DUMP_DIR = Path(
    r"C:\Users\chaho\ddoichaboom\Solo_Leveling\Resources\Models\map\Chap005\ThroneRoom\ThroneRoom_Split\dump"
)

# Use the explicit root transform from AssetStudio Related Assets first.
# The user-pasted Raw/Related Assets block corresponds to Transform #17313:
# identity TRS, 2414 children, father 13086.
ROOT_TRANSFORM_ID = 17313

# Fallback root name, kept only as a safety net when pathID is unavailable.
ROOT_NAME = "SortedStatics"

# Set to the collection that contains the imported FBX objects.
# Leave as None to scan every object in the scene.
TARGET_COLLECTION_NAME = None

# Only these object types are considered movable map instances.
MATCH_OBJECT_TYPES = {"MESH"}

# If True, only process dump groups related to the currently selected Blender objects.
# This is the safest and fastest mode. Select one clustered family first, then run.
ONLY_SELECTED_FAMILIES = True

# Minimum number of unique anchor pairs required to infer the global affine map.
MIN_ANCHORS = 6

# If True, skip anchor fitting and assume dump world space already matches Blender world space.
# Start with True for the current ThroneRoom case because the FBX already looks mostly correct.
FORCE_IDENTITY_AFFINE = True

# A repeated group is considered suspicious when its current Blender spread
# is much smaller than the spread implied by the dump transforms.
MIN_DUMP_SPREAD = 0.5
MAX_CURRENT_TO_TARGET_SPREAD_RATIO = 0.25

# Move only suspicious repeated groups by default.
MOVE_ONLY_SUSPICIOUS_REPEATED_GROUPS = True

# Fuzzy fallback can be expensive on large scenes and is risky.
# Leave disabled first. Turn on only if exact/canonical name matching misses too much.
USE_FUZZY_FALLBACK = False
MIN_FUZZY_SCORE = 35.0

# If True, also print groups that were detected but skipped.
VERBOSE = True


# ---------------------------------------------------------------------------
# Dump parsing
# ---------------------------------------------------------------------------

BLENDER_DUP_SUFFIX_RE = re.compile(r"\.\d{3}$")
LOD_SUFFIX_RE = re.compile(r"_lod\d+$", re.IGNORECASE)


@dataclass
class DumpNode:
    transform_id: int
    gameobject_name: str
    component_types: list[str]
    local_position: tuple[float, float, float]
    local_rotation_xyzw: tuple[float, float, float, float]
    local_scale: tuple[float, float, float]
    child_transform_ids: list[int]
    father_transform_id: int | None
    world_matrix_unity: list[list[float]] | None = None


def log(message: str) -> None:
    if VERBOSE:
        print(message)


def parse_path_id_from_filename(file_name: str) -> int | None:
    if "#" not in file_name:
        return None
    raw = file_name.split("#", 1)[1].rsplit(".", 1)[0].strip()
    return int(raw)


def normalize_exact_name(name: str) -> str:
    return BLENDER_DUP_SUFFIX_RE.sub("", name).strip()


def canonicalize_name(name: str) -> str:
    name = normalize_exact_name(name)
    lower = name.lower()

    # Blender object names sometimes include material segments like:
    # SM_DT_Arch_04_02_M_DT_Arch_01_LOD1
    # Remove the material chunk but keep the trailing number/LOD hints when possible.
    parts = lower.split("_")
    cleaned = []
    skip_material = False
    for index, token in enumerate(parts):
        if token == "m":
            skip_material = True
            continue

        if skip_material:
            is_last_lod = token.startswith("lod") and index == len(parts) - 1
            is_trailing_numeric = token.isdigit() and index == len(parts) - 2 and parts[-1].startswith("lod")
            if is_last_lod or is_trailing_numeric:
                skip_material = False
                cleaned.append(token)
            continue

        cleaned.append(token)

    compact = "_".join(token for token in cleaned if token)

    # If a name ends with "_01_lod0" and another ends with "_lod0", they are
    # still probably close enough to the same family. Keep both variants.
    return compact


def tokenize_name(name: str) -> tuple[str, ...]:
    return tuple(token for token in re.split(r"[^a-zA-Z0-9]+", canonicalize_name(name)) if token)


def similarity_score(left: str, right: str) -> float:
    left_tokens = set(tokenize_name(left))
    right_tokens = set(tokenize_name(right))
    if not left_tokens or not right_tokens:
        return 0.0

    common = left_tokens & right_tokens
    score = len(common) * 10.0

    left_c = canonicalize_name(left)
    right_c = canonicalize_name(right)
    if left_c == right_c:
        score += 100.0
    elif left_c in right_c or right_c in left_c:
        score += 25.0

    left_lod = extract_lod_token(left_c)
    right_lod = extract_lod_token(right_c)
    if left_lod is not None and right_lod is not None:
        if left_lod == right_lod:
            score += 15.0
        else:
            score -= 20.0

    return score


def names_are_related(left: str, right: str) -> bool:
    left_c = canonicalize_name(left)
    right_c = canonicalize_name(right)

    if left_c == right_c:
        return True
    if left_c in right_c or right_c in left_c:
        return True

    left_tokens = set(tokenize_name(left))
    right_tokens = set(tokenize_name(right))
    common = left_tokens & right_tokens
    return len(common) >= 3


def extract_lod_token(name: str) -> int | None:
    match = re.search(r"_lod(\d+)$", name, re.IGNORECASE)
    if match:
        return int(match.group(1))
    return None


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


def parse_gameobject_file(path: Path) -> tuple[str, list[int]]:
    data = path.read_bytes()
    component_count = struct.unpack_from("<i", data, 0)[0]
    offset = 4
    components = []
    for _ in range(component_count):
        path_id = struct.unpack_from("<q", data, offset + 4)[0]
        components.append(path_id)
        offset += 12

    offset += 4  # layer
    name_len = struct.unpack_from("<i", data, offset)[0]
    offset += 4
    name = data[offset : offset + name_len].decode("utf-8", errors="replace")
    return name, components


def parse_transform_file(path: Path):
    data = path.read_bytes()
    transform_id = parse_path_id_from_filename(path.name)
    if transform_id is None:
        raise ValueError(f"Transform file has no path id: {path}")

    gameobject_path_id = struct.unpack_from("<q", data, 4)[0]
    rotation_xyzw = struct.unpack_from("<4f", data, 12)
    position = struct.unpack_from("<3f", data, 28)
    scale = struct.unpack_from("<3f", data, 40)

    child_count = struct.unpack_from("<i", data, 52)[0]
    offset = 56
    child_ids = []
    for _ in range(child_count):
        child_path_id = struct.unpack_from("<q", data, offset + 4)[0]
        child_ids.append(child_path_id)
        offset += 12

    father_transform_id = None
    if offset + 12 <= len(data):
        father_transform_id = struct.unpack_from("<q", data, offset + 4)[0]

    return transform_id, gameobject_path_id, rotation_xyzw, position, scale, child_ids, father_transform_id


def load_dump_nodes(
    dump_dir: Path,
    root_name: str,
    root_transform_id: int | None = None,
) -> tuple[dict[int, DumpNode], int]:
    transform_dir = dump_dir / "Transform"
    gameobject_dir = dump_dir / "GameObject"

    transforms = {}
    for path in transform_dir.iterdir():
        if path.is_file() and path.suffix == ".dat" and "#" in path.name:
            record = parse_transform_file(path)
            transforms[record[0]] = record

    component_type_by_id = {}
    for subdir in dump_dir.iterdir():
        if not subdir.is_dir():
            continue
        if subdir.name in {"GameObject", "Transform"}:
            continue
        for path in subdir.iterdir():
            if not path.is_file() or path.suffix != ".dat":
                continue
            path_id = parse_path_id_from_filename(path.name)
            if path_id is not None:
                component_type_by_id[path_id] = subdir.name

    transform_to_go_name = {}
    transform_to_component_types = {}
    for path in gameobject_dir.iterdir():
        if not path.is_file() or path.suffix != ".dat":
            continue
        name, components = parse_gameobject_file(path)
        for component_id in components:
            if component_id in transforms:
                transform_to_go_name[component_id] = name
                transform_to_component_types[component_id] = [
                    component_type_by_id[cid]
                    for cid in components
                    if cid != component_id and cid in component_type_by_id
                ]

    nodes = {}
    for transform_id, record in transforms.items():
        _, gameobject_path_id, rotation_xyzw, position, scale, child_ids, father = record
        name = transform_to_go_name.get(transform_id, f"GameObjectPathID_{gameobject_path_id}")
        component_types = transform_to_component_types.get(transform_id, [])

        nodes[transform_id] = DumpNode(
            transform_id=transform_id,
            gameobject_name=name,
            component_types=component_types,
            local_position=position,
            local_rotation_xyzw=rotation_xyzw,
            local_scale=scale,
            child_transform_ids=list(child_ids),
            father_transform_id=father,
        )

    if root_transform_id is not None:
        if root_transform_id not in nodes:
            raise KeyError(f"Could not find dump root transform pathID {root_transform_id}")
        root_id = root_transform_id
    else:
        root_ids = [tid for tid, node in nodes.items() if node.gameobject_name == root_name]
        if not root_ids:
            raise KeyError(f"Could not find dump root named {root_name!r}")
        if len(root_ids) > 1:
            raise ValueError(f"Multiple dump roots named {root_name!r}: {root_ids}")
        root_id = root_ids[0]

    children_by_father = defaultdict(list)
    for tid, node in nodes.items():
        father = node.father_transform_id
        if father is not None and father != tid:
            children_by_father[father].append(tid)

    subtree = {}
    visited = set()

    def compute_world(tid: int) -> list[list[float]]:
        node = nodes[tid]
        if node.world_matrix_unity is not None:
            return node.world_matrix_unity
        local = trs_matrix(node.local_position, node.local_rotation_xyzw, node.local_scale)
        father = node.father_transform_id
        if father is None or father == tid or father not in nodes:
            world = local
        else:
            world = mat_mul(compute_world(father), local)
        node.world_matrix_unity = world
        return world

    def walk(tid: int):
        if tid in visited:
            return
        visited.add(tid)
        subtree[tid] = nodes[tid]
        for child_id in children_by_father.get(tid, []):
            walk(child_id)

    walk(root_id)
    for tid in list(subtree):
        compute_world(tid)

    return subtree, root_id


# ---------------------------------------------------------------------------
# Linear solve helpers
# ---------------------------------------------------------------------------

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
    if len(source_points) != len(target_points):
        raise ValueError("Source and target point counts differ.")
    if len(source_points) < 4:
        raise ValueError("Need at least 4 points to fit an affine transform.")

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


def max_distance_from_centroid(points: list[tuple[float, float, float]]) -> float:
    if not points:
        return 0.0
    cx = sum(point[0] for point in points) / len(points)
    cy = sum(point[1] for point in points) / len(points)
    cz = sum(point[2] for point in points) / len(points)
    return max(math.dist(point, (cx, cy, cz)) for point in points)


# ---------------------------------------------------------------------------
# Blender matching / application
# ---------------------------------------------------------------------------

def run_in_blender() -> None:
    try:
        import bpy
        from mathutils import Matrix
    except ImportError as exc:
        raise RuntimeError("This script must run inside Blender.") from exc

    dump_nodes, root_transform_id = load_dump_nodes(
        DUMP_DIR,
        ROOT_NAME,
        ROOT_TRANSFORM_ID,
    )
    root_node = dump_nodes[root_transform_id]
    mesh_nodes = [node for node in dump_nodes.values() if "MeshFilter" in node.component_types]
    dump_count_by_name = Counter(node.gameobject_name for node in mesh_nodes)

    if TARGET_COLLECTION_NAME is None:
        blender_objects = list(bpy.data.objects)
    else:
        collection = bpy.data.collections.get(TARGET_COLLECTION_NAME)
        if collection is None:
            raise KeyError(f"Collection not found: {TARGET_COLLECTION_NAME}")
        blender_objects = list(collection.all_objects)

    blender_objects = [obj for obj in blender_objects if obj.type in MATCH_OBJECT_TYPES]

    selected_family_names = set()
    if ONLY_SELECTED_FAMILIES and bpy.context.selected_objects:
        for obj in bpy.context.selected_objects:
            if obj.type not in MATCH_OBJECT_TYPES:
                continue
            selected_family_names.add(normalize_exact_name(obj.name))
            if obj.data is not None:
                selected_family_names.add(normalize_exact_name(obj.data.name))

    if selected_family_names:
        log(f"[ClusterFix] selected_families={sorted(selected_family_names)}")
    log(
        f"[ClusterFix] root_transform_id={root_transform_id} "
        f"root_name={root_node.gameobject_name} "
        f"root_children={len(root_node.child_transform_ids)}"
    )

    object_candidates_by_exact = defaultdict(list)
    object_candidates_by_canonical = defaultdict(list)
    def register_object_key(mapping, key, obj):
        if obj not in mapping[key]:
            mapping[key].append(obj)

    for obj in blender_objects:
        exact_names = {normalize_exact_name(obj.name)}
        if obj.data is not None:
            exact_names.add(normalize_exact_name(obj.data.name))

        for exact in exact_names:
            register_object_key(object_candidates_by_exact, exact, obj)
            register_object_key(object_candidates_by_canonical, canonicalize_name(exact), obj)

    # ------------------------------------------------------------------
    # Build anchors from unique names that match one Blender object.
    # ------------------------------------------------------------------
    if FORCE_IDENTITY_AFFINE:
        affine = [
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ]
        affine_matrix = Matrix(affine)
        log(f"[ClusterFix] dump_mesh_nodes={len(mesh_nodes)} blender_objects={len(blender_objects)}")
        log("[ClusterFix] using identity affine")
    else:
        anchor_pairs = []
        used_anchor_objects = set()

        for node in mesh_nodes:
            if dump_count_by_name[node.gameobject_name] != 1:
                continue

            candidates = list(object_candidates_by_exact.get(node.gameobject_name, []))
            if len(candidates) != 1:
                canonical = canonicalize_name(node.gameobject_name)
                candidates = list(object_candidates_by_canonical.get(canonical, []))

            if len(candidates) != 1:
                continue

            obj = candidates[0]
            if obj in used_anchor_objects:
                continue

            unity_pos = (
                node.world_matrix_unity[0][3],
                node.world_matrix_unity[1][3],
                node.world_matrix_unity[2][3],
            )
            blender_pos = tuple(obj.matrix_world.to_translation())
            anchor_pairs.append((unity_pos, blender_pos, node.gameobject_name, obj.name))
            used_anchor_objects.add(obj)

        log(f"[ClusterFix] dump_mesh_nodes={len(mesh_nodes)} blender_objects={len(blender_objects)}")
        log(f"[ClusterFix] unique_anchor_candidates={len(anchor_pairs)}")

        if len(anchor_pairs) < MIN_ANCHORS:
            raise RuntimeError(
                f"Not enough anchor pairs to infer affine transform. "
                f"Found {len(anchor_pairs)}, need at least {MIN_ANCHORS}."
            )

        source_points = [pair[0] for pair in anchor_pairs]
        target_points = [pair[1] for pair in anchor_pairs]
        affine = fit_affine_from_points(source_points, target_points)

        errors = []
        for src, dst, _, _ in anchor_pairs:
            mapped = transform_point_affine(affine, src)
            errors.append(math.dist(mapped, dst))
        avg_error = sum(errors) / len(errors)
        max_error = max(errors)

        affine_matrix = Matrix(affine)
        log(f"[ClusterFix] anchors={len(anchor_pairs)} avg_fit_error={avg_error:.4f} max_fit_error={max_error:.4f}")

    # ------------------------------------------------------------------
    # Group dump nodes by name and match to Blender objects.
    # ------------------------------------------------------------------
    dump_groups_by_name = defaultdict(list)
    for node in mesh_nodes:
        dump_groups_by_name[node.gameobject_name].append(node)

    matched_group_count = 0
    moved_group_count = 0
    skipped_group_count = 0
    filtered_out_group_count = 0

    for dump_name, nodes in sorted(dump_groups_by_name.items()):
        if len(nodes) <= 1:
            continue

        if selected_family_names:
            if not any(names_are_related(dump_name, family_name) for family_name in selected_family_names):
                filtered_out_group_count += 1
                continue

        objects = list(object_candidates_by_exact.get(dump_name, []))

        if not objects:
            canonical = canonicalize_name(dump_name)
            objects = list(object_candidates_by_canonical.get(canonical, []))

        if not objects and USE_FUZZY_FALLBACK:
            best_key = None
            best_score = 0.0
            for key in object_candidates_by_exact.keys():
                score = similarity_score(dump_name, key)
                if score > best_score:
                    best_score = score
                    best_key = key
            if best_key is not None and best_score >= MIN_FUZZY_SCORE:
                objects = list(object_candidates_by_exact[best_key])

        if not objects:
            skipped_group_count += 1
            log(f"[ClusterFix] skip no-match dump group: {dump_name} x{len(nodes)}")
            continue

        matched_group_count += 1

        objects = sorted(set(objects), key=lambda obj: obj.name)
        node_target_positions = [
            transform_point_affine(
                affine,
                (
                    node.world_matrix_unity[0][3],
                    node.world_matrix_unity[1][3],
                    node.world_matrix_unity[2][3],
                ),
            )
            for node in nodes
        ]

        current_positions = [tuple(obj.matrix_world.to_translation()) for obj in objects]
        current_spread = max_distance_from_centroid(current_positions)
        target_spread = max_distance_from_centroid(node_target_positions)

        suspicious = (
            target_spread >= MIN_DUMP_SPREAD
            and current_spread <= target_spread * MAX_CURRENT_TO_TARGET_SPREAD_RATIO
        )

        if MOVE_ONLY_SUSPICIOUS_REPEATED_GROUPS and not suspicious:
            skipped_group_count += 1
            log(
                f"[ClusterFix] keep group: {dump_name} "
                f"dump_count={len(nodes)} blender_count={len(objects)} "
                f"current_spread={current_spread:.3f} target_spread={target_spread:.3f}"
            )
            continue

        pair_count = min(len(nodes), len(objects))
        if pair_count == 0:
            skipped_group_count += 1
            continue

        nodes_sorted = sorted(
            nodes,
            key=lambda node: transform_point_affine(
                affine,
                (
                    node.world_matrix_unity[0][3],
                    node.world_matrix_unity[1][3],
                    node.world_matrix_unity[2][3],
                ),
            ),
        )
        objects_sorted = sorted(objects, key=lambda obj: obj.name)

        for index in range(pair_count):
            node = nodes_sorted[index]
            obj = objects_sorted[index]
            obj.matrix_world = affine_matrix @ Matrix(node.world_matrix_unity)

        moved_group_count += 1
        log(
            f"[ClusterFix] moved group: {dump_name} "
            f"dump_count={len(nodes)} blender_count={len(objects)} "
            f"pairs={pair_count} current_spread={current_spread:.3f} target_spread={target_spread:.3f}"
        )

    log(f"[ClusterFix] matched_repeated_groups={matched_group_count}")
    log(f"[ClusterFix] moved_repeated_groups={moved_group_count}")
    log(f"[ClusterFix] skipped_groups={skipped_group_count}")
    log(f"[ClusterFix] filtered_out_groups={filtered_out_group_count}")


if __name__ == "__main__":
    run_in_blender()
