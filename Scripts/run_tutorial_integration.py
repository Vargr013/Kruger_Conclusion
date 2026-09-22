"""Launch PIE, exercise camp interactions, then wait for a real practice shot."""
import runpy
import time
import unreal

script_dir = unreal.Paths.project_dir() + 'Scripts/'
runpy.run_path(script_dir + 'preview_tutorial.py')
integration_phase = 0
integration_next = 0

def coordinate(delta):
    global integration_phase, integration_next
    if time.monotonic() < integration_next:
        return
    game = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not game:
        return
    found = unreal.GameplayStatics.get_all_actors_of_class(game, unreal.PPTutorialDirector)
    if not found:
        return
    stage = found[0].get_stage()
    if integration_phase == 0 and stage == unreal.PPTutorialStage.ARRIVAL:
        runpy.run_path(script_dir + 'exercise_tutorial.py')
        integration_phase = 1
    elif integration_phase == 1 and stage == unreal.PPTutorialStage.PRACTICE:
        integration_phase = 2
        integration_next = time.monotonic() + 6
    elif integration_phase == 2:
        runpy.run_path(script_dir + 'exercise_tutorial_patrol.py')
        unreal.unregister_slate_post_tick_callback(integration_handle)

integration_handle = unreal.register_slate_post_tick_callback(coordinate)
