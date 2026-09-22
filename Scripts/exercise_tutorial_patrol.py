"""Exercise PIE event wiring and real escort navigation; not a human playtest.

Fire at the aimed target to start. The harness supplies restraint results and
steers the ranger; the captive must navigate and overlap the arrest zone itself.
"""
import time
import traceback
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
ranger = unreal.GameplayStatics.get_player_pawn(world, 0)
director = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PPTutorialDirector)[0]
target = director.get_editor_property('practice_target')
poacher = director.get_editor_property('tutorial_poacher')
zone = director.get_editor_property('arrest_zone')
assert director.get_stage() == unreal.PPTutorialStage.PRACTICE
pc.set_ignore_look_input(True)
pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(
    pc.player_camera_manager.get_camera_location(), target.get_actor_location()))
phase = 0
next_action = 0
deadline = time.monotonic() + 480
points = []
point_index = 1
last_log = 0

def stop():
    pc.reset_ignore_look_input()
    unreal.unregister_slate_post_tick_callback(patrol_handle)

def update(delta):
    global phase, next_action, points, point_index, last_log
    now = time.monotonic()
    if now < next_action:
        return
    try:
        stage = director.get_stage()
        if now > deadline:
            raise RuntimeError('Timed out in phase {} stage {}'.format(phase, stage))
        if now - last_log > 10:
            unreal.log('TUTORIAL PATROL phase={} stage={} poacher={}'.format(phase, stage, poacher.get_actor_location() if unreal.SystemLibrary.is_valid(poacher) else 'removed'))
            last_log = now
        if phase == 0 and stage == unreal.PPTutorialStage.PATROL:
            unreal.SystemLibrary.execute_console_command(world, 'Shot showui filename=TutorialPracticePassed.png', pc)
            ranger.set_actor_location(director.get_editor_property('patrol_destination').get_actor_location() + unreal.Vector(-400, -400, 40), False, True)
            phase = 1
            next_action = now + 2
        elif phase == 1 and stage == unreal.PPTutorialStage.SUBDUAL:
            poacher.apply_pepper_spray_slow()
            poacher.enter_subdued_state(30, True)
            assert pc.start_poacher_restraint(poacher), 'Restraint UI did not open'
            phase = 2
            next_action = now + 3
        elif phase == 2:
            assert unreal.GameplayStatics.is_game_paused(world), 'Restraint should pause training'
            widgets = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.PPRestraintMinigameWidget, False)
            assert len(widgets) == 1
            unreal.SystemLibrary.execute_console_command(world, 'Shot showui filename=TutorialRestraint.png', pc)
            widgets[0].on_restraint_finished.broadcast(unreal.PPRestraintResult.SUCCESS)
            assert director.get_stage() == unreal.PPTutorialStage.ESCORT
            # The return path was validated separately against rebuilt navigation.
            points = [
                ranger.get_actor_location(),
                unreal.Vector(114912, 27987, -190),
                unreal.Vector(112081, 26144, 60),
                unreal.Vector(109478, 23180, 130),
                unreal.Vector(106192, 21736, 85),
                zone.get_actor_location(),
            ]
            phase = 3
        elif phase == 3:
            if stage == unreal.PPTutorialStage.DEBRIEF:
                unreal.SystemLibrary.execute_console_command(world, 'Shot showui filename=TutorialDebrief.png', pc)
                phase = 4
                return
            assert stage == unreal.PPTutorialStage.ESCORT, 'Captive escaped or escort failed'
            position = ranger.get_actor_location()
            goal = points[point_index]
            direction = unreal.Vector(goal.x-position.x, goal.y-position.y, 0)
            distance = (direction.x**2 + direction.y**2)**0.5
            if distance < 100 and point_index < len(points)-1:
                point_index += 1
            elif distance >= 60:
                captive = poacher.get_actor_location()
                separation = ((position.x-captive.x)**2 + (position.y-captive.y)**2)**0.5
                if separation < 650:
                    ranger.add_movement_input(direction / max(distance, 1), 0.55, False)
                    pc.set_control_rotation(unreal.Rotator(pitch=0, yaw=unreal.MathLibrary.find_look_at_rotation(position, goal).yaw, roll=0))
        elif phase == 4 and stage == unreal.PPTutorialStage.COMPLETE:
            assert unreal.GameplayStatics.is_game_paused(world), 'Completion should show paused result screen'
            unreal.SystemLibrary.execute_console_command(world, 'Shot showui filename=TutorialComplete.png', pc)
            unreal.log('TUTORIAL INTEGRATION COMPLETE: weapon hit, resupply, restraint callback, navigated escort, arrest, cliffhanger')
            stop()
    except Exception:
        unreal.log_error(traceback.format_exc())
        stop()

patrol_handle = unreal.register_slate_post_tick_callback(update)
