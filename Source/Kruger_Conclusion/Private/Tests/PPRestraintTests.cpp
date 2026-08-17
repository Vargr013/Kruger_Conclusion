#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/PPPoacherCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UI/PPRestraintMinigameWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPRestraintRulesTest,
	"KrugerConclusion.PoachingPatrol.Restraint.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPRestraintRulesTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("An early click misses"), UPPRestraintMinigameWidget::IsHitTimingValid(0.74, 1.25, 0.50));
	TestTrue(TEXT("The generous early edge hits"), UPPRestraintMinigameWidget::IsHitTimingValid(0.75, 1.25, 0.50));
	TestTrue(TEXT("A click at alignment hits"), UPPRestraintMinigameWidget::IsHitTimingValid(1.25, 1.25, 0.50));
	TestTrue(TEXT("The generous late edge hits"), UPPRestraintMinigameWidget::IsHitTimingValid(1.75, 1.25, 0.50));
	TestFalse(TEXT("A click after the target expires misses"), UPPRestraintMinigameWidget::IsHitTimingValid(1.76, 1.25, 0.50));

	TestTrue(TEXT("Four hits remain possible with one target left"), UPPRestraintMinigameWidget::CanStillSucceed(3, 5, 6, 4));
	TestFalse(TEXT("Three misses make four hits impossible"), UPPRestraintMinigameWidget::CanStillSucceed(2, 5, 6, 4));
	TestEqual(TEXT("Normal approach duration is selected"), UPPRestraintMinigameWidget::GetApproachDurationForAssist(false, 1.25f, 1.75f), 1.25f);
	TestEqual(TEXT("Pepper-assisted approach duration is selected"), UPPRestraintMinigameWidget::GetApproachDurationForAssist(true, 1.25f, 1.75f), 1.75f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPRestraintPoacherStateTest,
	"KrugerConclusion.PoachingPatrol.Restraint.PoacherState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPRestraintPoacherStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("PPRestraintAutomationWorld"));
	if (!TestNotNull(TEXT("Automation world created"), World))
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	APPPoacherCharacter* FailedPoacher = World->SpawnActor<APPPoacherCharacter>();
	APPPoacherCharacter* SuccessfulPoacher = World->SpawnActor<APPPoacherCharacter>();
	APPPoacherCharacter* ResolvedPoacher = World->SpawnActor<APPPoacherCharacter>();
	AActor* Captor = World->SpawnActor<AActor>();

	if (TestNotNull(TEXT("Failed poacher exists"), FailedPoacher)
		&& TestNotNull(TEXT("Successful poacher exists"), SuccessfulPoacher)
		&& TestNotNull(TEXT("Resolved poacher exists"), ResolvedPoacher)
		&& TestNotNull(TEXT("Captor exists"), Captor))
	{
		TestTrue(TEXT("An active poacher can begin restraint"), FailedPoacher->CanStartCaptureAttempt());
		TestTrue(TEXT("The first begin succeeds"), FailedPoacher->BeginCaptureAttempt());
		TestTrue(TEXT("Attempt flag is set"), FailedPoacher->IsCaptureAttemptInProgress());
		TestFalse(TEXT("A duplicate begin is rejected"), FailedPoacher->BeginCaptureAttempt());
		TestTrue(TEXT("Failure resolves the active attempt"), FailedPoacher->ResolveCaptureAttempt(EPPRestraintResult::Failed, Captor));
		TestFalse(TEXT("Attempt flag clears after failure"), FailedPoacher->IsCaptureAttemptInProgress());
		TestTrue(TEXT("Failure activates the retry lock"), FailedPoacher->IsCaptureRetryLocked());
		TestFalse(TEXT("Failure does not capture the poacher"), FailedPoacher->IsCaptured());
		TestTrue(
			TEXT("Failure returns the poacher to a non-escort active state even when the isolated test world has no NavMesh"),
			FailedPoacher->GetPoacherState() == EPPPoacherState::Fleeing
				|| FailedPoacher->GetPoacherState() == EPPPoacherState::Alert
				|| FailedPoacher->GetPoacherState() == EPPPoacherState::DisguisedRoaming);
		TestFalse(TEXT("A duplicate resolution is ignored"), FailedPoacher->ResolveCaptureAttempt(EPPRestraintResult::Failed, Captor));

		TestTrue(TEXT("A second poacher can begin restraint"), SuccessfulPoacher->BeginCaptureAttempt());
		TestTrue(TEXT("Success resolves the active attempt"), SuccessfulPoacher->ResolveCaptureAttempt(EPPRestraintResult::Success, Captor));
		TestTrue(TEXT("Success captures the poacher"), SuccessfulPoacher->IsCaptured());
		TestEqual(TEXT("Success starts the existing escort state"), SuccessfulPoacher->GetPoacherState(), EPPPoacherState::FollowingPlayer);
		TestFalse(TEXT("A captured poacher cannot begin another attempt"), SuccessfulPoacher->CanStartCaptureAttempt());

		ResolvedPoacher->SetPoacherState(EPPPoacherState::Arrested);
		TestFalse(TEXT("An arrested poacher cannot begin restraint"), ResolvedPoacher->CanStartCaptureAttempt());
		ResolvedPoacher->SetPoacherState(EPPPoacherState::Escaped);
		TestFalse(TEXT("An escaped poacher cannot begin restraint"), ResolvedPoacher->CanStartCaptureAttempt());
	}

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
