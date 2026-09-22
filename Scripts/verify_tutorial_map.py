"""Read-only setup, affected Blueprint compilation, and navigation checks."""
import json
import unreal

MAP = '/Game/Poaching_Patrol/Levels/Skekuza_Level_Final'
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
unreal.WorldPartitionBlueprintLibrary.load_actors([
    desc.guid for desc in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
])
editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = editor.get_all_level_actors()
directors = [a for a in actors if isinstance(a, unreal.PPTutorialDirector)]
assert len(directors) == 1, 'Expected one tutorial director'
director = directors[0]
for name in ['tutorial_start', 'briefing_point', 'patrol_destination', 'resupply_point',
             'practice_target', 'tutorial_poacher', 'arrest_zone']:
    assert director.get_editor_property(name), 'Missing assignment: ' + name
for path in [
    '/Game/Poaching_Patrol/Assets/BP_LandCruiserRestPoint',
    '/Game/Poaching_Patrol/Assets/MyPPArrestZone',
    '/Game/Poaching_Patrol/Characters/Poachers/MyPPPoacherCharacter_Test',
]:
    bp = unreal.load_asset(path)
    assert bp, path
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.log('TUTORIAL BLUEPRINT compiled ' + path)
unreal.SystemLibrary.execute_console_command(world, 'MAP CHECK')
result = {'mode': str(director.get_editor_property('patrol_mode')), 'paths': []}
start = director.get_editor_property('tutorial_poacher').get_actor_location()
end = director.get_editor_property('arrest_zone').get_actor_location()
for name, source, target in [('escort', start, end), ('outbound', end, start)]:
    path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, source, target)
    entry = {'name': name, 'valid': bool(path and path.is_valid()),
             'partial': bool(path and path.is_partial()),
             'points': [str(p) for p in path.path_points] if path else []}
    result['paths'].append(entry)
    unreal.log('TUTORIAL PATH ' + json.dumps(entry))
with open(unreal.Paths.project_saved_dir() + 'Codex/tutorial_map_verification.json', 'w') as output:
    json.dump(result, output, indent=2)
unreal.log('TUTORIAL MAP CHECK FINISHED')
