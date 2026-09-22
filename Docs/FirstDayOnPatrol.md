# First Day on Patrol

The first playable tutorial lives in the blocked-off `First Day on Patrol` region of `Skekuza_Level_Final`. It reuses the map's landscape, trees, rocks, tents, resupply vehicle, restraint flow, escort behaviour, and arrest zone.

## Level configuration

The placed `PPTutorialDirector` is the pre-play level setting. Set `Patrol Mode` to `Tutorial` for the induction or `Normal Patrol` for the existing timed round. Tutorial mode starts after the opening menu closes, suppresses the patrol countdown and automatic round report, filters normal hostiles and wildlife, and owns the retry/completion flow.

The director exposes the tutorial start, briefing point, patrol destination, resupply point, practice target, poacher, arrest zone, objectives, hints, ranger lines, and optional voice clips. Map Check reports missing assignments, an incorrectly configured tutorial poacher, missing objectives, or multiple directors.

`Reviewed Community Opening` and `Reviewed Community Debrief` are intentionally empty. Add only team-approved engagement findings before dialogue recording.

## First playable sequence

The fixed sequence is Arrival, Briefing, Supply, Practice, Patrol, Subdual, Restraint, Escort, Debrief, Radio, and Complete. Progress comes from the assigned gameplay actors. Wrong actors, repeated hits, repeated resupplies, and repeated arrest notifications do not skip stages.

The tutorial poacher remains hidden, non-colliding, stationary, and AI-inactive until the player reaches the encounter. Restraint failure or escape returns guidance to the same poacher. Player defeat opens Retry Tutorial and Main Menu. Replay reloads the level so the director, encounter, dialogue, UI, and completion state start cleanly.

## Verification

Run `KrugerConclusion.PoachingPatrol` automation tests after gameplay changes. Rebuild World Partition navigation after moving the tutorial route, then compile the resupply, arrest-zone, and poacher Blueprints and run Map Check. The final acceptance pass remains a fresh-player packaged playthrough without coaching; use that pass to tune the current placeholder pacing toward the 8–12 minute target.
