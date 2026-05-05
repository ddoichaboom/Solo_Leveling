"""
Apply Unity AssetStudio raw dump transforms to imported Blender objects.

Intended workflow
1. Import the extracted FBX into Blender first.
2. Put the imported objects in one collection.
3. Adjust the config section below.
4. Run this script inside Blender's Text Editor.

What this script does
- Parses AssetStudio "Raw" export files from a dump folder.
- Rebuilds the hierarchy using Transform.m_Father.
- Computes Unity world transforms from local TRS.
- Converts Unity coordinates to Blender coordinates.
- Matches imported Blender objects by name and applies world transforms.
- Optionally creates an Empty hierarchy for inspection/debugging.

Notes
- Unity -> Blender basis conversion assumes:
  Unity: left-handed, Y-up, Z-forward
  Blender: right-handed, Z-up, -Y-forward
- Name matching strips Blender duplicate suffixes like ".001".
- If multiple dump nodes share the same name, objects are matched in sorted order.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from collections import defaultdict
import re
import struct


# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

DUMP_DIR = Path(
    r"C:\Users\chaho\ddoichaboom\Solo_Leveling\Resources\Models\map\Chap005\ThroneRoom\ThroneRoom_Split\dump"
)

ROOT_NAME = "SortedStatics"

# Imported FBX objects should live in this collection.
# Set to None to search every object in the current .blend.
TARGET_COLLECTION_NAME = None

# Create an Empty hierarchy that mirrors the Unity scene graph.
CREATE_HIERARCHY_EMPTIES = True
HIERARCHY_COLLECTION_NAME = f"{ROOT_NAME}_DumpHierarchy"

# Parent matched imported objects under their matching Empty while preserving world transform.
PARENT_MATCHED_OBJECTS_TO_EMPTY = True

# Restrict matching to mesh/empty objects. Usually safest for FBX imports.
MATCH_OBJECT_TYPES = {"MESH", "EMPTY"}

# If True, unmatched dump nodes still get Empty objects so you can inspect the layout.
CREATE_EMPTY_FOR_UNMATCHED_NODES = True

# If True, duplicate the first matched object when the dump has more instances than Blender.
# Leave False unless you know the imported FBX contains only one source mesh per shared name.
DUPLICATE_TEMPLATE_ON_OVERFLOW = False

# Keep verbose output in Blender's system console.
VERBOSE = True


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

BLENDER_DUP_SUFFIX_RE = re.compile(r"\.\d{3}$")


@dataclass
class GameObjectRecord:
    name: str
    components: list[int]
    layer: int
    file_path: Path


@dataclass
class TransformRecord:
    transform_id: int
    gameobject_path_id: int
    local_rotation_xyzw: tuple[float, float, float, float]
    local_position: tuple[float, float, float]
    local_scale: tuple[float, float, float]
    father_transform_id: int | None
    raw_children: list[int] = field(default_factory=list)
    children: list[int] = field(default_factory=list)


@dataclass
class SceneNode:
    transform_id: int
    name: str
    local_rotation_xyzw: tuple[float, float, float, float]
    local_position: tuple[float, float, float]
    local_scale: tuple[float, float, float]
    father_transform_id: int | None
    children: list[int]
    component_ids: list[int]
    component_types: list[str]


def normalize_name(name: str) -> str:
    stripped = BLENDER_DUP_SUFFIX_RE.sub("", name)
    return stripped.strip()


def parse_path_id_from_filename(file_name: str) -> int | None:
    if "#" not in file_name:
        return None
    raw = file_name.split("#", 1)[1].rsplit(".", 1)[0].strip()
    return int(raw)


def parse_gameobject_file(path: Path) -> GameObjectRecord:
    data = path.read_bytes()

    component_count = struct.unpack_from("<i", data, 0)[0]
    offset = 4
    components: list[int] = []
    for _ in range(component_count):
        path_id = struct.unpack_from("<q", data, offset + 4)[0]
        components.append(path_id)
        offset += 12

    layer = struct.unpack_from("<i", data, offset)[0]
    offset += 4

    name_len = struct.unpack_from("<i", data, offset)[0]
    offset += 4
    name = data[offset : offset + name_len].decode("utf-8", errors="replace")

    return GameObjectRecord(
        name=name,
        components=components,
        layer=layer,
        file_path=path,
    )


def parse_transform_file(path: Path) -> TransformRecord:
    data = path.read_bytes()
    transform_id = parse_path_id_from_filename(path.name)
    if transform_id is None:
        raise ValueError(f"Transform file has no path id: {path}")

    gameobject_path_id = struct.unpack_from("<q", data, 4)[0]
    rot = struct.unpack_from("<4f", data, 12)
    pos = struct.unpack_from("<3f", data, 28)
    scale = struct.unpack_from("<3f", data, 40)

    child_count = struct.unpack_from("<i", data, 52)[0]
    offset = 56
    raw_children: list[int] = []
    for _ in range(child_count):
        child_path_id = struct.unpack_from("<q", data, offset + 4)[0]
        raw_children.append(child_path_id)
        offset += 12

    father_transform_id = None
    if offset + 12 <= len(data):
        father_transform_id = struct.unpack_from("<q", data, offset + 4)[0]

    return TransformRecord(
        transform_id=transform_id,
        gameobject_path_id=gameobject_path_id,
        local_rotation_xyzw=rot,
        local_position=pos,
        local_scale=scale,
        father_transform_id=father_transform_id,
        raw_children=raw_children,
    )


def load_dump_scene(dump_dir: Path) -> dict[int, SceneNode]:
    transform_dir = dump_dir / "Transform"
    gameobject_dir = dump_dir / "GameObject"

    if not transform_dir.is_dir():
        raise FileNotFoundError(f"Missing Transform dir: {transform_dir}")
    if not gameobject_dir.is_dir():
        raise FileNotFoundError(f"Missing GameObject dir: {gameobject_dir}")

    transforms: dict[int, TransformRecord] = {}
    for path in transform_dir.iterdir():
        if path.is_file() and path.suffix == ".dat" and "#" in path.name:
            record = parse_transform_file(path)
            transforms[record.transform_id] = record

    component_type_by_id: dict[int, str] = {}
    for subdir in dump_dir.iterdir():
        if not subdir.is_dir():
            continue
        if subdir.name in {"GameObject", "Transform"}:
            continue
        for path in subdir.iterdir():
            if not path.is_file() or path.suffix != ".dat":
                continue
            path_id = parse_path_id_from_filename(path.name)
            if path_id is None:
                continue
            component_type_by_id[path_id] = subdir.name

    gameobjects: list[GameObjectRecord] = []
    transform_to_gameobject: dict[int, GameObjectRecord] = {}
    for path in gameobject_dir.iterdir():
        if not path.is_file() or path.suffix != ".dat":
            continue
        record = parse_gameobject_file(path)
        gameobjects.append(record)
        for component_id in record.components:
            if component_id in transforms:
                transform_to_gameobject[component_id] = record

    children_by_father: dict[int, list[int]] = defaultdict(list)
    for record in transforms.values():
        father = record.father_transform_id
        if father is not None:
            children_by_father[father].append(record.transform_id)

    nodes: dict[int, SceneNode] = {}
    for transform_id, transform in transforms.items():
        gameobject = transform_to_gameobject.get(transform_id)
        name = gameobject.name if gameobject is not None else f"GameObjectPathID_{transform.gameobject_path_id}"
        component_ids = gameobject.components[:] if gameobject is not None else [transform_id]
        component_types = [
            component_type_by_id[component_id]
            for component_id in component_ids
            if component_id != transform_id and component_id in component_type_by_id
        ]

        nodes[transform_id] = SceneNode(
            transform_id=transform_id,
            name=name,
            local_rotation_xyzw=transform.local_rotation_xyzw,
            local_position=transform.local_position,
            local_scale=transform.local_scale,
            father_transform_id=transform.father_transform_id,
            children=sorted(
                child_id
                for child_id in children_by_father.get(transform_id, [])
                if child_id != transform_id
            ),
            component_ids=component_ids,
            component_types=component_types,
        )

    return nodes


def find_root_transform_id(nodes: dict[int, SceneNode], root_name: str) -> int:
    matches = [node.transform_id for node in nodes.values() if node.name == root_name]
    if not matches:
        raise KeyError(f"Could not find root GameObject named {root_name!r}")
    if len(matches) > 1:
        raise ValueError(f"Multiple roots named {root_name!r}: {matches}")
    return matches[0]


# ---------------------------------------------------------------------------
# Blender integration
# ---------------------------------------------------------------------------

def run_in_blender() -> None:
    try:
        import bpy
        from mathutils import Matrix, Quaternion
    except ImportError as exc:
        raise RuntimeError("This script must run inside Blender.") from exc

    nodes = load_dump_scene(DUMP_DIR)
    root_transform_id = find_root_transform_id(nodes, ROOT_NAME)

    unity_to_blender = Matrix(
        (
            (1.0, 0.0, 0.0, 0.0),
            (0.0, 0.0, -1.0, 0.0),
            (0.0, 1.0, 0.0, 0.0),
            (0.0, 0.0, 0.0, 1.0),
        )
    )
    blender_to_unity = unity_to_blender.inverted()

    def local_matrix(node: SceneNode) -> Matrix:
        x, y, z, w = node.local_rotation_xyzw
        quat = Quaternion((w, x, y, z))
        rot = quat.to_matrix().to_4x4()
        trans = Matrix.Translation(node.local_position)
        scale = Matrix.Diagonal(
            (node.local_scale[0], node.local_scale[1], node.local_scale[2], 1.0)
        )
        return trans @ rot @ scale

    unity_world_by_id: dict[int, Matrix] = {}

    def build_unity_world(transform_id: int) -> Matrix:
        if transform_id in unity_world_by_id:
            return unity_world_by_id[transform_id]

        node = nodes[transform_id]
        father = node.father_transform_id
        local = local_matrix(node)

        if father is None or father == transform_id or father not in nodes:
            world = local
        else:
            world = build_unity_world(father) @ local

        unity_world_by_id[transform_id] = world
        return world

    subtree_ids: list[int] = []
    visited: set[int] = set()

    def collect_subtree(transform_id: int) -> None:
        if transform_id in visited:
            return
        visited.add(transform_id)
        subtree_ids.append(transform_id)
        for child_id in nodes[transform_id].children:
            collect_subtree(child_id)

    collect_subtree(root_transform_id)

    world_matrix_by_id: dict[int, Matrix] = {}
    for transform_id in subtree_ids:
        unity_world = build_unity_world(transform_id)
        world_matrix_by_id[transform_id] = unity_to_blender @ unity_world @ blender_to_unity

    def log(message: str) -> None:
        if VERBOSE:
            print(message)

    target_objects: list[object] = []
    if TARGET_COLLECTION_NAME is None:
        target_objects = list(bpy.data.objects)
    else:
        collection = bpy.data.collections.get(TARGET_COLLECTION_NAME)
        if collection is None:
            raise KeyError(f"Collection not found: {TARGET_COLLECTION_NAME}")
        target_objects = list(collection.all_objects)

    object_pool_by_name: dict[str, list[object]] = defaultdict(list)
    for obj in target_objects:
        if MATCH_OBJECT_TYPES and obj.type not in MATCH_OBJECT_TYPES:
            continue
        object_pool_by_name[normalize_name(obj.name)].append(obj)

    for pool in object_pool_by_name.values():
        pool.sort(key=lambda obj: obj.name)

    hierarchy_collection = None
    if CREATE_HIERARCHY_EMPTIES or CREATE_EMPTY_FOR_UNMATCHED_NODES:
        hierarchy_collection = bpy.data.collections.get(HIERARCHY_COLLECTION_NAME)
        if hierarchy_collection is None:
            hierarchy_collection = bpy.data.collections.new(HIERARCHY_COLLECTION_NAME)
            bpy.context.scene.collection.children.link(hierarchy_collection)

    empty_by_transform_id: dict[int, object] = {}
    matched_objects: list[tuple[int, object]] = []
    unmatched_nodes: list[int] = []
    overflow_duplicates = 0

    def ensure_empty(transform_id: int):
        if hierarchy_collection is None:
            return None
        empty = empty_by_transform_id.get(transform_id)
        if empty is not None:
            return empty

        node = nodes[transform_id]
        empty_name = f"{node.name}__T{transform_id}"
        empty = bpy.data.objects.new(empty_name, None)
        empty.empty_display_type = "PLAIN_AXES"
        empty.matrix_world = world_matrix_by_id[transform_id]
        hierarchy_collection.objects.link(empty)
        empty_by_transform_id[transform_id] = empty
        return empty

    if CREATE_HIERARCHY_EMPTIES:
        for transform_id in subtree_ids:
            ensure_empty(transform_id)

        for transform_id in subtree_ids:
            empty = empty_by_transform_id.get(transform_id)
            if empty is None:
                continue
            father = nodes[transform_id].father_transform_id
            if father is None or father == transform_id or father not in empty_by_transform_id:
                continue
            parent_empty = empty_by_transform_id[father]
            empty.parent = parent_empty
            empty.matrix_parent_inverse = parent_empty.matrix_world.inverted()

    used_count_by_name: dict[str, int] = defaultdict(int)

    for transform_id in subtree_ids:
        node = nodes[transform_id]
        normalized = normalize_name(node.name)
        pool = object_pool_by_name.get(normalized, [])
        used_index = used_count_by_name[normalized]
        target_obj = None

        if used_index < len(pool):
            target_obj = pool[used_index]
            used_count_by_name[normalized] += 1
        elif pool and DUPLICATE_TEMPLATE_ON_OVERFLOW:
            target_obj = pool[0].copy()
            if pool[0].data is not None:
                target_obj.data = pool[0].data.copy()
            if hierarchy_collection is not None:
                hierarchy_collection.objects.link(target_obj)
            else:
                bpy.context.scene.collection.objects.link(target_obj)
            overflow_duplicates += 1
        else:
            unmatched_nodes.append(transform_id)

        if target_obj is not None:
            target_obj.matrix_world = world_matrix_by_id[transform_id]
            matched_objects.append((transform_id, target_obj))

            if PARENT_MATCHED_OBJECTS_TO_EMPTY and transform_id in empty_by_transform_id:
                parent_empty = empty_by_transform_id[transform_id]
                target_obj.parent = parent_empty
                target_obj.matrix_parent_inverse = parent_empty.matrix_world.inverted()
                target_obj.matrix_world = world_matrix_by_id[transform_id]

    if CREATE_EMPTY_FOR_UNMATCHED_NODES and not CREATE_HIERARCHY_EMPTIES:
        for transform_id in unmatched_nodes:
            ensure_empty(transform_id)

    log(f"[UnityDump] root={ROOT_NAME} transform_id={root_transform_id}")
    log(f"[UnityDump] subtree_nodes={len(subtree_ids)}")
    log(f"[UnityDump] matched_objects={len(matched_objects)}")
    log(f"[UnityDump] unmatched_nodes={len(unmatched_nodes)}")
    log(f"[UnityDump] overflow_duplicates={overflow_duplicates}")

    if unmatched_nodes and VERBOSE:
        preview = unmatched_nodes[:50]
        for transform_id in preview:
            node = nodes[transform_id]
            log(
                f"  unmatched: T#{transform_id} {node.name} "
                f"components={node.component_types}"
            )
        if len(unmatched_nodes) > len(preview):
            log(f"  ... {len(unmatched_nodes) - len(preview)} more unmatched nodes")


if __name__ == "__main__":
    run_in_blender()
