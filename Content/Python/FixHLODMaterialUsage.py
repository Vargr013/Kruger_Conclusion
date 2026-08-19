import unreal


MATERIALS = [
    "/Game/Materials/TentNetting",
    "/Game/Materials/Tent",
    "/Game/Materials/Wood",
    "/Game/Materials/Wood2",
    "/Game/Materials/Wood3",
]


for material_path in MATERIALS:
    material = unreal.EditorAssetLibrary.load_asset(material_path)
    if material is None:
        raise RuntimeError("Failed to load HLOD source material: " + material_path)

    material.set_editor_property("used_with_instanced_static_meshes", True)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

unreal.log("OPTIMIZATION_HLOD_MATERIAL_USAGE_COMPLETE")
