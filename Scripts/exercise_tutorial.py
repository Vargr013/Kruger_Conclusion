"""PIE integration exercise; movements/hold are scripted, stage events remain real."""
import time
import traceback
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assert world, 'Start PIE first'
pc = unreal.GameplayStatics.get_player_controller(world, 0)
ranger = unreal.GameplayStatics.get_player_pawn(world, 0)
director = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PPTutorialDirector)[0]
rest = director.get_editor_property('resupply_point')
target = director.get_editor_property('practice_target')
phase = 0
deadline = time.monotonic() + 90
next_action = 0

def move_to(point, offset=unreal.Vector(0, 0, 0)):
    ranger.set_actor_location(point + offset, False, True)

def update(delta):
    global phase, next_action
    now = time.monotonic()
    if now < next_action:
        return
    try:
        if now > deadline:
            raise RuntimeError('Timed out at ' + str(director.get_stage()))
        stage = director.get_stage()
        if phase == 0:
            move_to(director.get_editor_property('briefing_point').get_actor_location())
            phase = 1
        elif phase == 1 and stage == unreal.PPTutorialStage.SUPPLY:
            move_to(rest.get_actor_location(), unreal.Vector(200, 0, 50))
            phase = 2
            next_action = now + 3
        elif phase == 2:
            assert rest.is_ranger_in_range_for(ranger), 'Vehicle overlap did not register'
            rest.advance_hold(3, True)
            assert director.get_stage() == unreal.PPTutorialStage.PRACTICE, 'Supply completion not received'
            move_to(target.get_actor_location(), unreal.Vector(0, -350, -60))
            phase = 3
            next_action = now + 3
        elif phase == 3:
            eyes = pc.player_camera_manager.get_camera_location()
            pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(eyes, target.get_actor_location()))
            unreal.log('TUTORIAL EXERCISE ready for practice shot; gun={} target={}'.format(
                ranger.get_current_gun(), target.get_actor_location()))
            unreal.unregister_slate_post_tick_callback(exercise_handle)
    except Exception:
        unreal.log_error(traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(exercise_handle)

exercise_handle = unreal.register_slate_post_tick_callback(update)
