#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Actors/PPTutorialDirector.h"
#include "Actors/PPTutorialTarget.h"
#include "Actors/PPRestPoint.h"
#include "Actors/PPArrestZone.h"
#include "Characters/ARangerCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "BaseProjectile.h"
#include "BaseGun.h"
#include "EnvironmentLevelSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPTutorialSequenceTest,
	"KrugerConclusion.PoachingPatrol.Tutorial.SequenceAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPTutorialSequenceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("TutorialTestWorld"));
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Director = World->SpawnActor<APPTutorialDirector>();
	auto* Ranger = World->SpawnActor<ARangerCharacter>();
	auto* PC = World->SpawnActor<APlayerController>();
	World->AddController(PC);
	PC->Possess(Ranger);
	TestNotNull(TEXT("Player pawn is available to gameplay queries"), UGameplayStatics::GetPlayerPawn(World, 0));
	Director->TutorialStart = World->SpawnActor<AActor>();
	Director->BriefingPoint = World->SpawnActor<AActor>();
	Director->PatrolDestination = World->SpawnActor<AActor>();
	Director->ResupplyPoint = World->SpawnActor<APPRestPoint>();
	Director->PracticeTarget = World->SpawnActor<APPTutorialTarget>();
	Director->TutorialPoacher = World->SpawnActor<APPPoacherCharacter>();
	Director->ArrestZone = World->SpawnActor<APPArrestZone>();
	Director->TutorialPoacher->bTutorialEncounter = true;
	Director->TutorialPoacher->bStartEncounterInactive = true;
	auto* OtherPoacher = World->SpawnActor<APPPoacherCharacter>();
	auto* OtherZone = World->SpawnActor<APPArrestZone>();
	auto* OtherTarget = World->SpawnActor<APPTutorialTarget>();
	auto* OtherRest = World->SpawnActor<APPRestPoint>();
	// I initialized the world so Unreal delivers dynamic gameplay delegates.
	World->InitializeActorsForPlay(FURL());
	auto* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	Rules->OnWorldBeginPlay(*World);
	Director->DispatchBeginPlay();
	TestTrue(TEXT("Director receives resupply events"), Director->ResupplyPoint->OnResupplyCompleted.IsBound());
	auto Advance = [&](float Seconds) { static_cast<AActor*>(Director)->Tick(Seconds); };
	TestEqual(TEXT("Menu has not started tutorial"), Director->GetStage(), EPPTutorialStage::WaitingForMenu);
	TestEqual(TEXT("Only assigned tutorial poacher registered"), Rules->GetRoundSnapshot().TotalPoachers, 1);
	Rules->ReportPoacherArrested(OtherPoacher);
	TestEqual(TEXT("Wrong mode actor cannot add arrests"), Rules->GetRoundSnapshot().PoachersArrested, 0);
	Rules->StartPatrol();
	TestEqual(TEXT("Menu dismissal starts arrival"), Director->GetStage(), EPPTutorialStage::Arrival);
	TestFalse(TEXT("Encounter remains inactive"), Director->TutorialPoacher->IsEncounterActive());
	TestEqual(TEXT("Inactive encounter cannot fall while collision is disabled"), Director->TutorialPoacher->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
	Advance(0.1f);
	TestEqual(TEXT("Arrival begins briefing"), Director->GetStage(), EPPTutorialStage::Briefing);
	Director->NotifyPracticeHit(Director->PracticeTarget, PC);
	TestEqual(TEXT("Out of order hit ignored"), Director->GetStage(), EPPTutorialStage::Briefing);
	Advance(30);
	TestEqual(TEXT("Briefing finishes before supply"), Director->GetStage(), EPPTutorialStage::Supply);
	Director->ResupplyPoint->OnResupplyCompleted.Broadcast(OtherRest, Ranger);
	TestEqual(TEXT("Wrong rest point ignored"), Director->GetStage(), EPPTutorialStage::Supply);
	Director->ResupplyPoint->SetOverlappingRanger(Ranger);
	Director->ResupplyPoint->AdvanceHold(3, true);
	TestEqual(TEXT("Resupply hold completed"), Director->ResupplyPoint->GetHoldProgress(), 1.0f);
	TestEqual(TEXT("Completed full-health resupply accepted"), Director->GetStage(), EPPTutorialStage::Practice);
	Director->ResupplyPoint->AdvanceHold(3, true);
	TestEqual(TEXT("Duplicate hold cannot skip practice"), Director->GetStage(), EPPTutorialStage::Practice);
	Director->NotifyPracticeHit(OtherTarget, PC);
	TestEqual(TEXT("Wrong target ignored"), Director->GetStage(), EPPTutorialStage::Practice);
	auto* Gun = World->SpawnActor<ABaseGun>();
	Gun->SetOwner(Ranger);
	Gun->SetInstigator(Ranger);
	UGameplayStatics::ApplyDamage(Director->PracticeTarget, 20, Gun->GetInstigatorController(), Gun, nullptr);
	TestEqual(TEXT("Player weapon attribution completes practice"), Director->GetStage(), EPPTutorialStage::Patrol);
	Director->NotifyPracticeHit(Director->PracticeTarget, PC);
	TestEqual(TEXT("Duplicate weapon hit cannot skip patrol"), Director->GetStage(), EPPTutorialStage::Patrol);
	Advance(0.1f);
	TestEqual(TEXT("Reaching encounter begins subdual"), Director->GetStage(), EPPTutorialStage::Subdual);
	TestEqual(TEXT("Active encounter restores walking"), Director->TutorialPoacher->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);
	OtherPoacher->EnterSubduedState(0, true);
	TestEqual(TEXT("Wrong poacher cannot advance"), Director->GetStage(), EPPTutorialStage::Subdual);
	auto* Poacher = Director->TutorialPoacher.Get();
	Poacher->EnterSubduedState(0, true);
	TestEqual(TEXT("Assigned subdual begins restraint"), Director->GetStage(), EPPTutorialStage::Restraint);
	Poacher->BeginCaptureAttempt();
	Poacher->ResolveCaptureAttempt(EPPRestraintResult::Failed, Ranger);
	Advance(0.1f);
	TestEqual(TEXT("Restraint failure recovers to subdual"), Director->GetStage(), EPPTutorialStage::Subdual);
	Poacher->EnterSubduedState(0, true);
	Poacher->CapturePoacher(Ranger);
	TestEqual(TEXT("Capture begins escort"), Director->GetStage(), EPPTutorialStage::Escort);
	Poacher->EscapePoacher();
	TestEqual(TEXT("Escape returns guidance to same poacher"), Director->GetStage(), EPPTutorialStage::Subdual);
	Poacher->SetPoacherState(EPPPoacherState::Alert);
	Poacher->EnterSubduedState(0, true);
	Poacher->CapturePoacher(Ranger);
	TestFalse(TEXT("Wrong zone cannot consume captive"), Director->AllowsArrest(Poacher, OtherZone));
	Director->NotifyArrest(Poacher, OtherZone);
	TestEqual(TEXT("Wrong zone cannot finish escort"), Director->GetStage(), EPPTutorialStage::Escort);
	TestTrue(TEXT("Assigned zone accepts captive"), Director->AllowsArrest(Poacher, Director->ArrestZone));
	Poacher->MarkArrested();
	Rules->ReportPoacherArrested(Poacher);
	Director->NotifyArrest(Poacher, Director->ArrestZone);
	Director->NotifyArrest(Poacher, Director->ArrestZone);
	TestEqual(TEXT("Arrest begins debrief exactly once"), Director->GetStage(), EPPTutorialStage::Debrief);
	TestFalse(TEXT("Final arrest does not end normal round"), Rules->HasRoundEnded());
	Advance(40);
	TestEqual(TEXT("Debrief precedes radio"), Director->GetStage(), EPPTutorialStage::Radio);
	Advance(3); Advance(20); Advance(20);
	TestEqual(TEXT("Closing dialogue completes tutorial"), Director->GetStage(), EPPTutorialStage::Complete);
	Rules->ReportPlayerDowned();
	TestEqual(TEXT("Late defeat cannot overwrite completion"), Director->GetStage(), EPPTutorialStage::Complete);
	TestFalse(TEXT("Tutorial never generated newspaper result"), Rules->HasRoundEnded());
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPTutorialNormalRulesTest,
	"KrugerConclusion.PoachingPatrol.Tutorial.NormalPatrolSeparation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPPTutorialNormalRulesTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("TutorialNormalTest"));
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Director = World->SpawnActor<APPTutorialDirector>();
	Director->PatrolMode = EPPPatrolMode::NormalPatrol;
	auto* Normal = World->SpawnActor<APPPoacherCharacter>();
	auto* Training = World->SpawnActor<APPPoacherCharacter>(); Training->bTutorialEncounter = true;
	auto* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	Rules->OnWorldBeginPlay(*World);
	TestFalse(TEXT("Level is normal patrol"), Rules->IsTutorialMode());
	TestEqual(TEXT("Training encounter excluded from normal quota"), Rules->GetRoundSnapshot().TotalPoachers, 1);
	Rules->StartPatrol();
	TestTrue(TEXT("Normal timer starts"), Rules->HasPatrolStarted());
	TestTrue(TEXT("Normal timer has remaining time"), Rules->GetPatrolSecondsRemaining() > 0);
	Rules->ReportPoacherArrested(Normal);
	TestTrue(TEXT("Normal final arrest ends round"), Rules->HasRoundEnded());
	TestEqual(TEXT("Tutorial remains unstarted"), Director->GetStage(), EPPTutorialStage::WaitingForMenu);
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
