import unreal


actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
nav_meshes = [
    actor
    for actor in actor_subsystem.get_all_level_actors()
    if isinstance(actor, unreal.RecastNavMesh)
]
if not nav_meshes:
    raise RuntimeError("No RecastNavMesh actor was loaded")

for nav_mesh in nav_meshes:
    nav_mesh.modify()
    nav_mesh.set_editor_property("fixed_tile_pool_size", True)
    nav_mesh.set_editor_property("tile_pool_size", 16016)
    nav_mesh.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.STATIC)
    unreal.log(
        "OPTIMIZATION_NAV_CONFIGURED actor="
        + nav_mesh.get_path_name()
        + " tile_pool_size="
        + str(nav_mesh.get_editor_property("tile_pool_size"))
    )

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.EditorLoadingAndSavingUtils.save_map(
    nav_meshes[0].get_world(), nav_meshes[0].get_world().get_path_name().split(".")[0]
)
