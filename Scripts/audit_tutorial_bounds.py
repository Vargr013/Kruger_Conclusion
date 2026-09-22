import json
import unreal

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Poaching_Patrol/Levels/Skekuza_Level_Final')
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
rows = []
for actor in actors:
    if actor.actor_has_tag('PPTutorial'):
        origin, extent = actor.get_actor_bounds(False)
        rows.append({'label': actor.get_actor_label(), 'origin': str(origin), 'extent': str(extent), 'scale': str(actor.get_actor_scale3d())})
with open(unreal.Paths.project_saved_dir() + 'Codex/tutorial_bounds.json', 'w') as output:
    json.dump(rows, output, indent=2)
