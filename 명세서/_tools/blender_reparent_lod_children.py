"""
Reparent imported FBX LOD mesh objects under their matching anchor objects.

Why this exists
- Some imported families already have the expected structure:
  ParentName
    ParentName_LOD0
- Other families are imported as flat siblings:
  ParentName
  ParentName_LOD0
- In the broken case, the mesh child keeps its local zero transform but loses the
  parent link, so it stays clustered at the wrong world position.

What this script does
- Finds objects named like `Something_LOD0.001`
- Looks for a matching anchor object named `Something.001`
- Reparents the LOD object under that anchor
- Preserves the child's local transform instead of preserving world transform

Usage
1. Open this script in Blender's Text Editor.
2. Optionally select a few broken objects first and enable ONLY_SELECTED_FAMILIES.
3. Run the script.
4. Check the console log for repaired and skipped items.
"""

import re

import bpy
from mathutils import Matrix


TARGET_COLLECTION_NAME = None

# Safer for first runs. If objects are selected, only related name families are processed.
ONLY_SELECTED_FAMILIES = False

# Repair objects even if they currently have the wrong parent.
REPAIR_WRONG_PARENTS = True

# Only treat mesh objects as LOD children by default.
CHILD_TYPES = {"MESH"}

LOD_NAME_RE = re.compile(r"^(?P<base>.+?)_LOD(?P<lod>\d+)(?P<suffix>\.\d{3})?$", re.IGNORECASE)
BLENDER_DUP_SUFFIX_RE = re.compile(r"(?P<stem>.*?)(?P<suffix>\.\d{3})?$")


def log(message):
    print(message)


def iter_target_objects():
    if TARGET_COLLECTION_NAME is None:
        return list(bpy.data.objects)

    collection = bpy.data.collections.get(TARGET_COLLECTION_NAME)
    if collection is None:
        raise KeyError(f"Collection not found: {TARGET_COLLECTION_NAME}")
    return list(collection.all_objects)


def split_duplicate_suffix(name):
    match = BLENDER_DUP_SUFFIX_RE.match(name)
    if not match:
        return name, ""
    return match.group("stem"), match.group("suffix") or ""


def extract_selected_bases():
    bases = set()
    if not ONLY_SELECTED_FAMILIES:
        return bases

    for obj in bpy.context.selected_objects:
        match = LOD_NAME_RE.match(obj.name)
        if match:
            bases.add(match.group("base"))
        else:
            stem, _ = split_duplicate_suffix(obj.name)
            bases.add(stem)
    return bases


def preserve_local_reparent(child, parent):
    location = child.location.copy()
    scale = child.scale.copy()
    rotation_mode = child.rotation_mode

    if rotation_mode == "QUATERNION":
        rotation_value = child.rotation_quaternion.copy()
    elif rotation_mode == "AXIS_ANGLE":
        rotation_value = tuple(child.rotation_axis_angle)
    else:
        rotation_value = child.rotation_euler.copy()

    child.parent = parent
    child.parent_type = "OBJECT"
    child.matrix_parent_inverse = Matrix.Identity(4)

    child.location = location
    child.scale = scale
    child.rotation_mode = rotation_mode
    if rotation_mode == "QUATERNION":
        child.rotation_quaternion = rotation_value
    elif rotation_mode == "AXIS_ANGLE":
        child.rotation_axis_angle = rotation_value
    else:
        child.rotation_euler = rotation_value


def main():
    objects = iter_target_objects()
    objects_by_name = {obj.name: obj for obj in objects}
    selected_bases = extract_selected_bases()

    repaired = 0
    already_ok = 0
    skipped_missing_parent = 0
    skipped_wrong_type = 0
    skipped_filtered = 0
    skipped_has_other_parent = 0

    for child in objects:
        if child.type not in CHILD_TYPES:
            continue

        match = LOD_NAME_RE.match(child.name)
        if not match:
            continue

        base = match.group("base")
        suffix = match.group("suffix") or ""
        expected_parent_name = f"{base}{suffix}"

        if selected_bases and base not in selected_bases and expected_parent_name not in selected_bases:
            skipped_filtered += 1
            continue

        parent = objects_by_name.get(expected_parent_name)
        if parent is None:
            skipped_missing_parent += 1
            continue

        if parent == child:
            skipped_missing_parent += 1
            continue

        if child.parent == parent:
            already_ok += 1
            continue

        if child.parent is not None and child.parent != parent and not REPAIR_WRONG_PARENTS:
            skipped_has_other_parent += 1
            continue

        preserve_local_reparent(child, parent)
        repaired += 1
        log(f"[ReparentLOD] repaired child={child.name} parent={parent.name}")

    log(
        f"[ReparentLOD] repaired={repaired} "
        f"already_ok={already_ok} "
        f"missing_parent={skipped_missing_parent} "
        f"wrong_type={skipped_wrong_type} "
        f"filtered={skipped_filtered} "
        f"other_parent_skipped={skipped_has_other_parent}"
    )


if __name__ == "__main__":
    main()
