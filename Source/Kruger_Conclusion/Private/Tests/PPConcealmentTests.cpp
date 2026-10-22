#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Data/PPNoiseEmitterComponent.h"
#include "Data/PPHealthComponent.h"
#include "Actors/PPVegetationPatch.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Characters/ARangerCharacter.h"

namespace
{
UWorld* CoverTestWorld()
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	return World;
}
void DestroyCoverWorld(UWorld* World)
{
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
}
UBoxComponent* AddWall(UWorld* World, FVector Centre)
{
	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(20, 200, 300));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Box->RegisterComponent();
	Box->SetWorldLocation(Centre);
	return Box;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPConcealmentSightTest,
	"KrugerConclusion.PoachingPatrol.Stealth.SightAndPassableCover", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPPConcealmentSightTest::RunTest(const FString& Parameters)
{
	UWorld* World = CoverTestWorld();
	APPPoacherCharacter* Observer = World->SpawnActor<APPPoacherCharacter>();
	APPAnimalCharacter* Target = World->SpawnActor<APPAnimalCharacter>();
	Observer->SetActorLocation(FVector(0, 0, 96));
	Target->SetActorLocation(FVector(800, 0, 96));
	TestTrue(TEXT("Uncovered target is visible"), Observer->CanSeeTarget(Target));
	UBoxComponent* Wall = AddWall(World, FVector(400, 0, 100));
	TestFalse(TEXT("Solid wall blocks sight"), Observer->CanSeeTarget(Target));
	Target->SetActorLocation(FVector(150, 0, 96));
	Wall->SetWorldLocation(FVector(75, 0, 100));
	TestFalse(TEXT("Near contact does not bypass walls"), Observer->CanSeeTarget(Target));
	Wall->DestroyComponent();
	Target->SetActorLocation(FVector(800, 0, 96));
	APPVegetationPatch* Patch = World->SpawnActor<APPVegetationPatch>();
	TestTrue(TEXT("Gameplay cover preserves rendering and collision settings"), Patch->HasProtectedCoverSettings());
	Patch->AddSightCore(FVector(730, 0, 62), FVector(100, 120, 62));
	TestTrue(TEXT("Standing head remains above tall grass"), Observer->CanSeeTarget(Target));
	Target->GetCapsuleComponent()->SetCapsuleHalfHeight(48);
	Target->SetIsCrouched(true);
	Target->RecalculateBaseEyeHeight();
	Target->SetActorLocation(FVector(800, 0, 48));
	TestFalse(TEXT("Crouched body is concealed by tall grass"), Observer->CanSeeTarget(Target));
	Patch->ClearPatch();
	Patch->AddSightCore(FVector(400, 0, 110), FVector(100, 120, 110));
	Target->SetIsCrouched(false);
	Target->RecalculateBaseEyeHeight();
	Target->SetActorLocation(FVector(800, 0, 96));
	TestFalse(TEXT("Bush core conceals standing body"), Observer->CanSeeTarget(Target));
	for (const UBoxComponent* Core : Patch->SightCores)
	{
		TestEqual(TEXT("Cover does not block pawns"), Core->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
		TestEqual(TEXT("Cover does not block spray or interaction"), Core->GetCollisionResponseToChannel(ECC_Visibility), ECR_Ignore);
		TestEqual(TEXT("Cover does not block projectiles"), Core->GetCollisionResponseToChannel(ECC_GameTraceChannel1), ECR_Ignore);
		TestFalse(TEXT("Cover does not affect navigation"), Core->CanEverAffectNavigation());
	}
	Target->SetActorLocation(FVector(150, 0, 96));
	Patch->ClearPatch();
	Patch->AddSightCore(FVector(75, 0, 110), FVector(25, 120, 110));
	TestTrue(TEXT("Nearby target can be noticed through vegetation"), Observer->CanSeeTarget(Target));
	DestroyCoverWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPConcealmentNoiseTest,
	"KrugerConclusion.PoachingPatrol.Stealth.NoiseSnapshotsAndExpiry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPPConcealmentNoiseTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Stationary is silent"), UPPNoiseEmitterComponent::MovementRadius(false, false, 0), 0.0f);
	TestEqual(TEXT("Crouching radius"), UPPNoiseEmitterComponent::MovementRadius(true, false, 100), 200.0f);
	TestEqual(TEXT("Walking radius"), UPPNoiseEmitterComponent::MovementRadius(false, false, 600), 600.0f);
	TestEqual(TEXT("Sprinting radius"), UPPNoiseEmitterComponent::MovementRadius(false, true, 900), 1200.0f);
	UWorld* World = CoverTestWorld();
	APPPoacherCharacter* Observer = World->SpawnActor<APPPoacherCharacter>();
	APPAnimalCharacter* Source = World->SpawnActor<APPAnimalCharacter>();
	Observer->SetActorLocation(FVector(0, 0, 100));
	Source->SetActorLocation(FVector(-800, 0, 100));
	FPPThreatObservation Observation;
	TestFalse(TEXT("A stationary unseen source is not heard"), Observer->ObserveTarget(Source, Observation));
	Source->NoiseEmitter->EmitNoise(1200);
	Source->SetActorLocation(FVector(-1100, 200, 100));
	TestTrue(TEXT("Recent sound is heard behind observer"), Observer->ObserveTarget(Source, Observation));
	TestFalse(TEXT("Hearing grants no sight"), Observation.bSeen);
	TestEqual(TEXT("Sound stays at emission position"), Observation.Location, FVector(-800, 0, 100));
	Observer->RememberObservation(Source, Observation);
	TestEqual(TEXT("Memory stores the stimulus position"), Observer->GetLastObservedLocation(), FVector(-800, 0, 100));
	UBoxComponent* Wall = AddWall(World, FVector(-400, 0, 100));
	TestFalse(TEXT("Solid wall halves hearing range"), Observer->ObserveTarget(Source, Observation));
	Wall->DestroyComponent();
	for (int32 Step = 0; Step < 12; ++Step) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestFalse(TEXT("Old action sound expires"), Observer->ObserveTarget(Source, Observation));
	TestEqual(TEXT("Unseen movement cannot update memory"), Observer->GetLastObservedLocation(), FVector(-800, 0, 100));
	for (int32 Step = 0; Step < 62; ++Step) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestFalse(TEXT("Pursuit memory expires after seven seconds"), Observer->FollowRememberedTarget());
	Observer->ClearTargetMemory();
	TestTrue(TEXT("Cleared observation has no timestamp"), Observer->GetLastObservedTime() < 0);
	Source->NoiseEmitter->EmitNoise(1200);
	Source->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Source->GetCharacterMovement()->Velocity = FVector(100, 0, 0);
	Source->SetIsCrouched(true);
	Source->NoiseEmitter->TickComponent(0.1f, LEVELTICK_All, nullptr);
	TArray<FPPNoiseStimulus> Stimuli;
	Source->NoiseEmitter->GetActiveStimuli(Stimuli);
	TestEqual(TEXT("Movement and action noises have separate records"), Stimuli.Num(), 2);
	TestTrue(TEXT("Quiet steps do not overwrite a shot"), Stimuli.ContainsByPredicate([](const FPPNoiseStimulus& Noise) { return Noise.Radius == 1200; }));
	DestroyCoverWorld(World);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPPredatorWarningTest,
	"KrugerConclusion.PoachingPatrol.Stealth.PredatorWindupAndCancellation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPPPredatorWarningTest::RunTest(const FString& Parameters)
{
	UWorld* World = CoverTestWorld();
	UClass* PredatorClass = LoadClass<APPAnimalCharacter>(nullptr,
		TEXT("/Game/Poaching_Patrol/Characters/Animals/MyPPAnimalCharacter_Predator.MyPPAnimalCharacter_Predator_C"));
	if (!TestNotNull(TEXT("Actual predator Blueprint loads"), PredatorClass)) { DestroyCoverWorld(World); return false; }
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APPAnimalCharacter* Predator = World->SpawnActor<APPAnimalCharacter>(PredatorClass, FVector(0, 0, 96), FRotator::ZeroRotator, Spawn);
	APPAnimalCharacter* Prey = World->SpawnActor<APPAnimalCharacter>(APPAnimalCharacter::StaticClass(), FVector(170, 0, 96), FRotator::ZeroRotator, Spawn);
	Predator->DispatchBeginPlay(); Prey->DispatchBeginPlay();
	Predator->StopAIUpdates(); Prey->StopAIUpdates();
	Predator->GetCharacterMovement()->DisableMovement(); Prey->GetCharacterMovement()->DisableMovement();
	const float Health = Prey->GetHealthComponent()->GetCurrentHealth();
	Predator->UpdateCreatureAI();
	TestTrue(TEXT("Predator warns before striking"), Predator->IsPredatorAttackPending());
	TestEqual(TEXT("Preparation does not instantly damage prey"), Prey->GetHealthComponent()->GetCurrentHealth(), Health);
	Prey->SetActorLocation(FVector(600, 0, 96));
	for (int32 Step = 0; Step < 7; ++Step) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestEqual(TEXT("Leaving range cancels the hit"), Prey->GetHealthComponent()->GetCurrentHealth(), Health);
	for (int32 Step = 0; Step < 15; ++Step) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	Prey->SetActorLocation(FVector(170, 0, 96));
	Predator->UpdateCreatureAI();
	for (int32 Step = 0; Step < 7; ++Step) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestTrue(TEXT("A reachable visible target is damaged after warning"), Prey->GetHealthComponent()->GetCurrentHealth() < Health);
	const float RemainingHealth = Prey->GetHealthComponent()->GetCurrentHealth();
	for (int32 Step = 0; Step < 15; ++Step) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	Predator->UpdateCreatureAI();
	UBoxComponent* Wall = AddWall(World, FVector(85, 0, 100));
	for (int32 Step = 0; Step < 7; ++Step) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestEqual(TEXT("Solid cover during windup cancels predator damage"), Prey->GetHealthComponent()->GetCurrentHealth(), RemainingHealth);
	Wall->DestroyComponent();
	Predator->StopAIUpdates();
	TestFalse(TEXT("Stopping AI clears pending strikes"), Predator->IsPredatorAttackPending());
	DestroyCoverWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPPoacherLostTargetTest,
	"KrugerConclusion.PoachingPatrol.Stealth.PoacherUsesLastObservation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPPPoacherLostTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = CoverTestWorld();
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerController* PC = World->SpawnActor<APlayerController>();
	ARangerCharacter* Ranger = World->SpawnActor<ARangerCharacter>(ARangerCharacter::StaticClass(), FVector(500,0,96), FRotator::ZeroRotator, Spawn);
	PC->Possess(Ranger);
	APPPoacherCharacter* Poacher = World->SpawnActor<APPPoacherCharacter>(APPPoacherCharacter::StaticClass(), FVector(0,0,96), FRotator::ZeroRotator, Spawn);
	Poacher->DispatchBeginPlay(); Poacher->StopAIUpdates();
	Poacher->UpdateCreatureAI();
	TestEqual(TEXT("Visible ranger starts engagement"), Poacher->GetPoacherState(), EPPPoacherState::EngagingPlayer);
	TestEqual(TEXT("Visible position was remembered"), Poacher->GetLastObservedLocation(), FVector(500,0,96));
	UBoxComponent* Wall = AddWall(World, FVector(250,0,100));
	Ranger->SetActorLocation(FVector(900,300,96));
	Poacher->UpdateCreatureAI();
	TestEqual(TEXT("Lost sight keeps the old move destination"), Poacher->GetCurrentMoveTarget(), FVector(500,0,96));
	TestEqual(TEXT("Unreachable remembered destination returns safely to idle"), Poacher->GetPoacherState(), EPPPoacherState::Alert);
	Wall->DestroyComponent(); Ranger->SetActorLocation(FVector(500,0,96));
	Poacher->UpdateCreatureAI();
	TestEqual(TEXT("Sight reacquisition resumes engagement"), Poacher->GetPoacherState(), EPPPoacherState::EngagingPlayer);
	Poacher->EnterSubduedState(4);
	TestTrue(TEXT("Subdual clears pursuit observations"), Poacher->GetLastObservedTime()<0);
	TestFalse(TEXT("Subdual clears pending attacks"), Poacher->IsPlayerAttackWindupActive());
	DestroyCoverWorld(World);
	return true;
}
#endif
