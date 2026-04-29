import bpy
from pathlib import Path
from collections import defaultdict


TEXTURE_DIR = Path(
    r"C:\Users\chaho\ddoichaboom\Solo_Leveling\Resources\Models\map\Chap005\ThroneRoom\ThroneRoom_merge"
)

IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".tif", ".tiff", ".dds"}
ROLE_SUFFIXES = {
    "_D": "base_color",
    "_N": "normal",
    "_S": "specular",
    "_E": "emission",
}
ROLE_LABELS = {
    "base_color": "Auto_BaseColor",
    "normal": "Auto_Normal",
    "specular": "Auto_Specular",
    "emission": "Auto_Emission",
}
EMISSION_STRENGTH_MULTIPLIER = 5.0
SPECULAR_MAP_DRIVES_ROUGHNESS = True


def strip_extension(name: str) -> str:
    return Path(name).stem if "." in name else name


def normalize_name(name: str) -> str:
    return "".join(ch.lower() for ch in strip_extension(name) if ch.isalnum())


def split_role(stem: str):
    upper_stem = stem.upper()
    for suffix, role in ROLE_SUFFIXES.items():
        if upper_stem.endswith(suffix):
            return stem[: -len(suffix)], role
    return stem, None


def build_texture_index(texture_dir: Path):
    by_filename = {}
    by_stem = {}
    by_base = defaultdict(dict)

    for path in texture_dir.iterdir():
        if not path.is_file() or path.suffix.lower() not in IMAGE_EXTENSIONS:
            continue

        by_filename[path.name.lower()] = path
        by_stem[path.stem.lower()] = path

        base_stem, role = split_role(path.stem)
        if role is not None:
            by_base[base_stem.lower()][role] = path

    return by_filename, by_stem, by_base


def load_image(path: Path):
    return bpy.data.images.load(str(path), check_existing=True)


def repair_image_datablocks(by_filename):
    repaired = 0
    for image in bpy.data.images:
        candidates = []
        if image.filepath:
            candidates.append(Path(bpy.path.abspath(image.filepath)).name)
        if image.name:
            candidates.append(Path(image.name).name)

        for candidate in candidates:
            path = by_filename.get(candidate.lower())
            if path is None:
                continue
            image.filepath = str(path)
            image.reload()
            repaired += 1
            break

    return repaired


def get_materials_from_selection():
    materials = []
    seen = set()

    objects = bpy.context.selected_objects
    if not objects:
        objects = [obj for obj in bpy.data.objects if obj.type == "MESH"]

    for obj in objects:
        for slot in obj.material_slots:
            mat = slot.material
            if mat is None or mat.node_tree is None or mat.name in seen:
                continue
            seen.add(mat.name)
            materials.append(mat)

    return materials


def ensure_principled(mat):
    for node in mat.node_tree.nodes:
        if node.type == "BSDF_PRINCIPLED":
            return node
    return None


def guess_base_candidates(mat):
    candidates = []

    for node in mat.node_tree.nodes:
        if node.type != "TEX_IMAGE":
            continue

        if node.image is not None:
            if node.image.name:
                candidates.append(strip_extension(Path(node.image.name).name))
            if node.image.filepath:
                candidates.append(strip_extension(Path(bpy.path.abspath(node.image.filepath)).name))

        if node.label:
            candidates.append(strip_extension(node.label))
        if node.name:
            candidates.append(strip_extension(node.name))

    candidates.append(strip_extension(mat.name))
    return candidates


def fuzzy_find_texture_set(mat, by_base):
    raw_candidates = guess_base_candidates(mat)
    exact_hits = []

    for candidate in raw_candidates:
        base_stem, _ = split_role(candidate)
        if base_stem.lower() in by_base:
            exact_hits.append(base_stem.lower())

    if exact_hits:
        counts = defaultdict(int)
        for hit in exact_hits:
            counts[hit] += 1
        return max(counts.items(), key=lambda item: item[1])[0]

    mat_norm = normalize_name(mat.name)
    best_base = None
    best_score = 0

    for base in by_base.keys():
        base_norm = normalize_name(base)
        if not base_norm:
            continue

        score = 0
        if mat_norm == base_norm:
            score += 100
        elif mat_norm in base_norm or base_norm in mat_norm:
            score += 50

        shared = sum(1 for ch in set(base_norm) if ch in mat_norm)
        score += shared

        if score > best_score:
            best_score = score
            best_base = base

    return best_base if best_score >= 10 else None


def find_image_node_by_role(mat, role):
    for node in mat.node_tree.nodes:
        if node.type != "TEX_IMAGE":
            continue

        names = [node.name, node.label]
        if node.image is not None:
            names.append(node.image.name)

        for name in names:
            if not name:
                continue
            _, detected_role = split_role(strip_extension(Path(name).name))
            if detected_role == role:
                return node

    return None


def ensure_image_node(mat, role, image_path: Path):
    node = find_image_node_by_role(mat, role)
    if node is None:
        node = mat.node_tree.nodes.new("ShaderNodeTexImage")
        node.label = ROLE_LABELS[role]
        node.name = f"{ROLE_LABELS[role]}_{image_path.stem}"
        node.location = (-700, {
            "base_color": 300,
            "normal": 0,
            "specular": -300,
            "emission": -600,
        }[role])

    node.image = load_image(image_path)
    return node


def ensure_link(links, from_socket, to_socket):
    for link in links:
        if link.from_socket == from_socket and link.to_socket == to_socket:
            return
    links.new(from_socket, to_socket)


def clear_input_links(node_tree, socket):
    for link in list(node_tree.links):
        if link.to_socket == socket:
            node_tree.links.remove(link)


def get_linked_source_socket(node_tree, socket):
    for link in node_tree.links:
        if link.to_socket == socket:
            return link.from_socket
    return None


def ensure_named_node(mat, bl_idname, name, location):
    for node in mat.node_tree.nodes:
        if node.bl_idname == bl_idname and node.name == name:
            return node

    node = mat.node_tree.nodes.new(bl_idname)
    node.name = name
    node.label = name
    node.location = location
    return node


def connect_base_color(mat, bsdf, image_node):
    image_node.image.colorspace_settings.name = "sRGB"
    ensure_link(mat.node_tree.links, image_node.outputs["Color"], bsdf.inputs["Base Color"])


def connect_normal(mat, bsdf, image_node):
    image_node.image.colorspace_settings.name = "Non-Color"

    normal_node = None
    for node in mat.node_tree.nodes:
        if node.type == "NORMAL_MAP":
            normal_node = node
            break

    if normal_node is None:
        normal_node = mat.node_tree.nodes.new("ShaderNodeNormalMap")
        normal_node.location = (-350, 0)

    ensure_link(mat.node_tree.links, image_node.outputs["Color"], normal_node.inputs["Color"])
    ensure_link(mat.node_tree.links, normal_node.outputs["Normal"], bsdf.inputs["Normal"])


def connect_specular(mat, bsdf, image_node):
    image_node.image.colorspace_settings.name = "Non-Color"

    target_name = None
    if "Specular IOR Level" in bsdf.inputs:
        target_name = "Specular IOR Level"
    elif "Specular" in bsdf.inputs:
        target_name = "Specular"

    if target_name is not None:
        rgb_to_bw = ensure_named_node(
            mat,
            "ShaderNodeRGBToBW",
            "Auto_Specular_ToBW",
            (-450, -300),
        )
        ensure_link(mat.node_tree.links, image_node.outputs["Color"], rgb_to_bw.inputs["Color"])
        ensure_link(mat.node_tree.links, rgb_to_bw.outputs["Val"], bsdf.inputs[target_name])

    if SPECULAR_MAP_DRIVES_ROUGHNESS and "Roughness" in bsdf.inputs:
        if get_linked_source_socket(mat.node_tree, bsdf.inputs["Roughness"]) is None:
            rgb_to_bw = ensure_named_node(
                mat,
                "ShaderNodeRGBToBW",
                "Auto_Roughness_ToBW",
                (-450, -380),
            )
            invert = ensure_named_node(
                mat,
                "ShaderNodeMath",
                "Auto_Roughness_Invert",
                (-250, -380),
            )
            invert.operation = "SUBTRACT"
            invert.inputs[0].default_value = 1.0

            ensure_link(mat.node_tree.links, image_node.outputs["Color"], rgb_to_bw.inputs["Color"])
            ensure_link(mat.node_tree.links, rgb_to_bw.outputs["Val"], invert.inputs[1])
            ensure_link(mat.node_tree.links, invert.outputs["Value"], bsdf.inputs["Roughness"])


def connect_emission(mat, bsdf, image_node):
    image_node.image.colorspace_settings.name = "sRGB"

    target_name = None
    if "Emission Color" in bsdf.inputs:
        target_name = "Emission Color"
    elif "Emission" in bsdf.inputs:
        target_name = "Emission"

    if target_name is not None:
        color_input = bsdf.inputs[target_name]
        base_color_source = get_linked_source_socket(mat.node_tree, bsdf.inputs["Base Color"])

        # Many exported game materials use emissive maps as masks.
        # Prefer the base color as emission color when available, otherwise fall back to the emissive texture itself.
        clear_input_links(mat.node_tree, color_input)
        if base_color_source is not None:
            ensure_link(mat.node_tree.links, base_color_source, color_input)
        else:
            ensure_link(mat.node_tree.links, image_node.outputs["Color"], color_input)

    if "Emission Strength" in bsdf.inputs:
        rgb_to_bw = ensure_named_node(
            mat,
            "ShaderNodeRGBToBW",
            "Auto_Emission_ToBW",
            (-450, -600),
        )
        multiply = ensure_named_node(
            mat,
            "ShaderNodeMath",
            "Auto_Emission_Strength",
            (-250, -600),
        )
        multiply.operation = "MULTIPLY"
        multiply.inputs[1].default_value = EMISSION_STRENGTH_MULTIPLIER

        ensure_link(mat.node_tree.links, image_node.outputs["Color"], rgb_to_bw.inputs["Color"])
        ensure_link(mat.node_tree.links, rgb_to_bw.outputs["Val"], multiply.inputs[0])
        clear_input_links(mat.node_tree, bsdf.inputs["Emission Strength"])
        ensure_link(mat.node_tree.links, multiply.outputs["Value"], bsdf.inputs["Emission Strength"])


def reconnect_material(mat, by_base):
    bsdf = ensure_principled(mat)
    if bsdf is None:
        return False, f"{mat.name}: no Principled BSDF"

    base_key = fuzzy_find_texture_set(mat, by_base)
    if base_key is None:
        return False, f"{mat.name}: no texture set match"

    role_paths = by_base.get(base_key, {})
    if not role_paths:
        return False, f"{mat.name}: matched set empty"

    linked_any = False

    if "base_color" in role_paths:
        node = ensure_image_node(mat, "base_color", role_paths["base_color"])
        connect_base_color(mat, bsdf, node)
        linked_any = True

    if "normal" in role_paths:
        node = ensure_image_node(mat, "normal", role_paths["normal"])
        connect_normal(mat, bsdf, node)
        linked_any = True

    if "specular" in role_paths:
        node = ensure_image_node(mat, "specular", role_paths["specular"])
        connect_specular(mat, bsdf, node)
        linked_any = True

    if "emission" in role_paths:
        node = ensure_image_node(mat, "emission", role_paths["emission"])
        connect_emission(mat, bsdf, node)
        linked_any = True

    return linked_any, f"{mat.name}: {base_key}"


def main():
    if not TEXTURE_DIR.exists():
        raise FileNotFoundError(f"Texture directory not found: {TEXTURE_DIR}")

    by_filename, by_stem, by_base = build_texture_index(TEXTURE_DIR)
    materials = get_materials_from_selection()
    repaired_images = repair_image_datablocks(by_filename)

    print("=== Texture Directory Summary ===")
    print(f"Texture directory: {TEXTURE_DIR}")
    print(f"FBX expected beside textures: {TEXTURE_DIR / 'ThroneRoom.fbx'}")
    print(f"Texture files indexed: {len(by_filename)}")
    print("Texture sets:")
    for base, role_map in sorted(by_base.items()):
        print(f"  {base} -> {', '.join(sorted(role_map.keys()))}")

    print("\n=== Material Reconnect ===")
    print(f"Materials to inspect: {len(materials)}")
    print(f"Image datablocks path-repaired: {repaired_images}")

    linked_count = 0
    for mat in materials:
        linked, message = reconnect_material(mat, by_base)
        print(("OK   " if linked else "SKIP ") + message)
        if linked:
            linked_count += 1

    print("\n=== Suffix Meaning ===")
    print("  _D -> Base Color / Diffuse")
    print("  _N -> Normal")
    print("  _S -> Specular (may need manual adjustment if the game packed smoothness here)")
    print("  _E -> Emission")
    print("  M_DT_Balco_2559/2560/2562 -> unknown by name only; script only path-repairs existing references")

    print("\n=== Result ===")
    print(f"Materials auto-linked: {linked_count}/{len(materials)}")


if __name__ == "__main__":
    main()
