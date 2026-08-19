import unreal


TARGET_ACTOR_NAME = "StaticMeshActor_UAID_BCECA026B87980D202_1907661785"

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target = next(
    (actor for actor in actor_subsystem.get_all_level_actors() if actor.get_name() == TARGET_ACTOR_NAME),
    None,
)
if target is None:
    raise RuntimeError("Map Check actor was not loaded: " + TARGET_ACTOR_NAME)

component = target.static_mesh_component
target.modify()
component.modify()
body_instance = component.get_editor_property("body_instance")
body_instance.set_editor_property("collision_enabled", unreal.CollisionEnabled.NO_COLLISION)
component.set_editor_property("body_instance", body_instance)
target.set_actor_enable_collision(False)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.EditorLoadingAndSavingUtils.save_map(target.get_world(), target.get_world().get_path_name().split(".")[0])
unreal.log(
    "OPTIMIZATION_MAP_WARNING_FIXED collision="
    + str(component.get_collision_enabled())
    + " actor="
    + target.get_path_name()
)
