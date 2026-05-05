"""
Set Emission Strength to 0 on materials in Blender.

Default behavior:
- If objects are selected, only materials on the selection are processed.
- If nothing is selected, all materials in the file are processed.
"""

import bpy


ONLY_SELECTED_OBJECTS = True


def iter_target_materials():
    seen = set()

    if ONLY_SELECTED_OBJECTS and bpy.context.selected_objects:
        objects = bpy.context.selected_objects
    else:
        objects = bpy.data.objects

    for obj in objects:
        for slot in obj.material_slots:
            mat = slot.material
            if mat is None or mat.name in seen:
                continue
            seen.add(mat.name)
            yield mat


def zero_emission_strength(material):
    if material.node_tree is None:
        return False

    changed = False
    for node in material.node_tree.nodes:
        if node.type == "BSDF_PRINCIPLED" and "Emission Strength" in node.inputs:
            socket = node.inputs["Emission Strength"]
            if socket.default_value != 0.0:
                socket.default_value = 0.0
                changed = True

        if node.type == "EMISSION" and "Strength" in node.inputs:
            socket = node.inputs["Strength"]
            if socket.default_value != 0.0:
                socket.default_value = 0.0
                changed = True

    return changed


def main():
    changed_count = 0
    material_count = 0

    for material in iter_target_materials():
        material_count += 1
        if zero_emission_strength(material):
            changed_count += 1

    print(
        f"[ZeroEmissionStrength] scanned_materials={material_count} "
        f"changed_materials={changed_count}"
    )


if __name__ == "__main__":
    main()
