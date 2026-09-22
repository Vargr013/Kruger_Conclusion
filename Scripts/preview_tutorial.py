"""Open the saved tutorial in PIE for visual and interaction verification."""
import time
import traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
level.load_level('/Game/Poaching_Patrol/Levels/Skekuza_Level_Final')
unreal.EditorLevelLibrary.set_level_viewport_camera_info(
    unreal.Vector(103400, 19800, 300), unreal.Rotator(pitch=-5, yaw=-45, roll=0))
phase = 0
next_action = time.monotonic() + 5

def update(delta):
    global phase, next_action
    now = time.monotonic()
    if now < next_action:
        return
    try:
        if phase == 0:
            level.editor_request_begin_play()
            phase = 1
            next_action = now + 15
            return
        world = editor.get_game_world()
        if not world:
            next_action = now + 2
            return
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        if not pc:
            next_action = now + 2
            return
        if phase == 1:
            for widget in unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.UserWidget, False):
                if 'MainMenuOverlay' in widget.get_class().get_name():
                    widget.remove_from_parent()
            unreal.GameplayStatics.set_game_paused(world, False)
            pc.set_editor_property('show_mouse_cursor', False)
            unreal.WidgetLibrary.set_input_mode_game_only(pc)
            phase = 2
            next_action = now + 15
        else:
            directors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PPTutorialDirector)
            unreal.log('TUTORIAL PREVIEW stage={} player={}'.format(
                directors[0].get_stage(), unreal.GameplayStatics.get_player_pawn(world, 0).get_actor_location()))
            unreal.SystemLibrary.execute_console_command(world, 'Shot showui filename=TutorialArrival.png', pc)
            unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        unreal.log_error(traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(handle)

handle = unreal.register_slate_post_tick_callback(update)
