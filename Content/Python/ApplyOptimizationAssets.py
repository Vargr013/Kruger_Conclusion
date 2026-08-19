import unreal


SOURCE_TYPES = [
    "/Game/Foliage/Grass1_FoliageType",
    "/Game/Foliage/Grass2_FoliageType",
    "/Game/Foliage/Grass3_FoliageType",
]
TARGET_TYPES = [
    "/Game/Poaching_Patrol/Environment/Foliage/FT_PP_Grass1",
    "/Game/Poaching_Patrol/Environment/Foliage/FT_PP_Grass2",
    "/Game/Poaching_Patrol/Environment/Foliage/FT_PP_Grass3",
]


for source, target in zip(SOURCE_TYPES, TARGET_TYPES):
    if not unreal.EditorAssetLibrary.does_asset_exist(target):
        if not unreal.EditorAssetLibrary.duplicate_asset(source, target):
            raise RuntimeError("Failed to duplicate foliage type: " + source)
    foliage_type = unreal.EditorAssetLibrary.load_asset(target)
    foliage_type.set_editor_property("enable_density_scaling", True)
    foliage_type.set_editor_property("enable_cull_distance_scaling", True)
    foliage_type.set_editor_property("include_in_hlod", False)
    unreal.EditorAssetLibrary.save_loaded_asset(foliage_type, only_if_is_dirty=False)
    if not unreal.PPOptimizationEditorLibrary.replace_foliage_type_in_editor(source, target):
        raise RuntimeError("Failed to remap foliage type: " + source)

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log("OPTIMIZATION_FOLIAGE_REMAP_COMPLETE")
