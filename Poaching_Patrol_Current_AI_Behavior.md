# Poaching Patrol Current AI Behavior

## Summary
- The current AI foundation supports animals and poachers.
- Both inherit shared movement, threat detection, fleeing, roaming, idle wandering, and debug behavior from `APPCreatureBase`.
- AI updates are timer-driven through `AIUpdateInterval`, not heavy per-frame Tick logic.
- Movement uses `AAIController::MoveToLocation` and navmesh-based random destination selection.

## Shared Creature Behavior
- Shared base class: `APPCreatureBase`.
- Default movement:
  - `WalkSpeed = 240`
  - `FleeSpeed = 420`
  - `RoamRadius = 650`
  - `RoamAcceptanceRadius = 80`
- AI update timing:
  - `AIUpdateInterval = 0.25`
  - `MoveRequestCooldown = 0.75`
- Move failure recovery:
  - `MaxConsecutiveMoveFailures = 3`
  - After 3 failed move requests, the current target is cleared.
  - This lets the next AI update choose a fresh location instead of hanging.
- Stale movement recovery:
  - If the AI controller is no longer path-following, the current move is treated as finished.
  - This prevents creatures waiting forever on abandoned paths.

## Idle And Roaming Loop
- Idle no longer means standing completely still.
- Current loop:
  - Stand for a short time.
  - Wander in a small local radius.
  - Stand again.
  - Repeat local wandering until the idle window ends.
  - Roam to a broader destination.
  - Repeat.
- Shared idle tuning:
  - `IdleLocalWanderRadius = 220`
  - `IdleLocalWanderAcceptanceRadius = 70`
  - `IdleStandTimeMin = 1.0`
  - `IdleStandTimeMax = 2.25`
- Animal idle window:
  - `IdleTimeMin = 4.0`
  - `IdleTimeMax = 7.0`
- Poacher idle window:
  - `IdleTimeMin = 3.5`
  - `IdleTimeMax = 6.5`

## Threat Detection
- Threat detection is shared through `FindBestThreatActor()`.
- The selected threat is the closest valid detected threat.
- Detection modes:
  - Sight: longer range, forward cone.
  - Sound/proximity: shorter range, all directions.
- Shared threat defaults:
  - `SightThreatRadius = 900`
  - `SightThreatAngleDegrees = 90`
  - `SoundThreatRadius = 350`
  - `bUseSightThreats = true`
  - `bUseSoundThreats = true`
- The 90-degree sight angle is treated as a total cone:
  - 45 degrees left.
  - 45 degrees right.

## Animal Behavior
- Class: `APPAnimalCharacter`.
- Animal states:
  - `Idle`
  - `Roaming`
  - `Alert`
  - `Fleeing`
- Animals flee from:
  - Player pawn.
  - Poachers.
  - Animals with a different species tag.
- Animals ignore:
  - Themselves.
  - Animals with the same valid `AnimalSpeciesTag`.
- Animals cannot be captured.
- Animals do not implement the interactable interface.

## Poacher Behavior
- Class: `APPPoacherCharacter`.
- Poacher states:
  - `DisguisedRoaming`
  - `Alert`
  - `Fleeing`
  - `Captured`
  - `FollowingPlayer`
  - `Arrested`
  - `Escaped`
- Poachers flee from:
  - Player pawn.
  - Animals.
- Poachers ignore:
  - Themselves.
  - Other poachers by default.
- Poachers implement `PPInteractableInterface`.
- Player raycast interaction can capture poachers.

## Flee Behavior
- Fleeing is triggered by the best valid detected threat.
- Flee destination is projected onto the navmesh.
- Fleeing uses the creature's `FleeSpeed`.
- Flee duration is randomized:
  - `FleeDurationMin = 5.0`
  - `FleeDurationMax = 10.0`
- Once fleeing starts, the AI keeps fleeing until the flee timer ends.
- If a different valid threat becomes the best threat, the AI can restart fleeing from that new threat.

## Capture And Escort Behavior
- Captured poachers:
  - Store the captor as `CaptorActor`.
  - Switch to `FollowingPlayer`.
  - Use `CapturedMoveSpeed`.
  - Follow when outside `FollowDistance`.
- Default follow tuning:
  - `FollowDistance = 250`
  - `FollowAcceptanceRadius = 120`
  - `CapturedMoveSpeed = 220`
- Escort pressure:
  - `EscapeDistance = 900`
  - `EscapeGraceTime = 5.0`
  - `bCanEscapeAfterCapture = true`
- If the captor stays too far away:
  - `EscapeProgress` increases.
  - At the grace limit, the poacher escapes.
  - Escaped poachers flee for the randomized flee duration.
  - After the escape burst, they return to disguised roaming.
- Arrest behavior:
  - `APPArrestZone` arrests captured/following poachers on overlap.
  - Arrested poachers stop moving.
  - Arrested poachers cannot escape.
  - Arrested poachers ignore further interaction.

## Debug Behavior
- Debug output is controlled by `bDrawDebug`.
- On-screen messages are controlled by `bDebugOnScreen`.
- Debug messages include:
  - State changes.
  - Threat selection.
  - Threat checks.
  - Roam destinations.
  - Idle local wander destinations.
  - Flee start and duration.
  - Capture, escape, and arrest states.
  - Move request failures.
- Debug gizmos include:
  - Sight radius.
  - Sound/proximity radius.
  - Forward sight cone edges.
  - Forward direction arrow.
  - Threat line when a threat is being checked.

## Current Scope Exclusions
- No shop system.
- No economy.
- No full round manager.
- No split unit system.
- No suspicion UI.
- No predators.
- No poaching sites.
- No animal capture.
