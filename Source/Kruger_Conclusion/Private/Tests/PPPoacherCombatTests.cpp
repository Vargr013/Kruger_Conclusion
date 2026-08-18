#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/PPPoacherCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnvironmentLevelSubsystem.h"

namespace
{
	UWorld* CreateCombatTestWorld(const TCHAR* Name)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(Name));
		if (World)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyCombatTestWorld(UWorld* World)
	{
		if (World)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPPoacherSubdualRulesTest,
	"KrugerConclusion.PoachingPatrol.Combat.SubdualAndRestraint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPPoacherSubdualRulesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCombatTestWorld(TEXT("PPPoacherSubdualAutomationWorld"));
	if (!TestNotNull(TEXT("Automation world created"), World))
	{
		return false;
	}

	APPPoacherCharacter* Poacher = World->SpawnActor<APPPoacherCharacter>();
	AActor* Captor = World->SpawnActor<AActor>();
	if (TestNotNull(TEXT("Poacher exists"), Poacher) && TestNotNull(TEXT("Captor exists"), Captor))
	{
		TestFalse(TEXT("Healthy poacher rejects restraint"), Poacher->CanStartCaptureAttempt());
		TestEqual(TEXT("Healthy prompt asks for subdual"), Poacher->GetInteractionPrompt_Implementation().ToString(), FString(TEXT("Subdue Poacher First")));

		Poacher->EnterSubduedState(4.0f, false);
		TestTrue(TEXT("Pepper-style subdual is active"), Poacher->IsSubdued());
		TestEqual(TEXT("Subdual uses the explicit state"), Poacher->GetPoacherState(), EPPPoacherState::Subdued);
		TestTrue(TEXT("Subdued poacher accepts restraint"), Poacher->BeginCaptureAttempt());
		TestTrue(TEXT("Successful restraint resolves"), Poacher->ResolveCaptureAttempt(EPPRestraintResult::Success, Captor));
		TestTrue(TEXT("Successful restraint captures"), Poacher->IsCaptured());
		TestEqual(TEXT("Capture preserves escort state"), Poacher->GetPoacherState(), EPPPoacherState::FollowingPlayer);
		TestFalse(TEXT("Captured poacher cannot attack-windup"), Poacher->IsPlayerAttackWindupActive());
	}

	DestroyCombatTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPPoacherAttackSlotTest,
	"KrugerConclusion.PoachingPatrol.Combat.AttackSlotLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPPoacherAttackSlotTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCombatTestWorld(TEXT("PPPoacherAttackSlotAutomationWorld"));
	if (!TestNotNull(TEXT("Automation world created"), World))
	{
		return false;
	}

	UEnvironmentLevelSubsystem* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	APPPoacherCharacter* PoacherA = World->SpawnActor<APPPoacherCharacter>();
	APPPoacherCharacter* PoacherB = World->SpawnActor<APPPoacherCharacter>();
	APPPoacherCharacter* PoacherC = World->SpawnActor<APPPoacherCharacter>();
	if (TestNotNull(TEXT("Rules exist"), Rules)
		&& TestNotNull(TEXT("First poacher exists"), PoacherA)
		&& TestNotNull(TEXT("Second poacher exists"), PoacherB)
		&& TestNotNull(TEXT("Third poacher exists"), PoacherC))
	{
		TestTrue(TEXT("First poacher gets a slot"), Rules->TryAcquirePlayerAttackSlot(PoacherA));
		TestTrue(TEXT("Second poacher gets a slot"), Rules->TryAcquirePlayerAttackSlot(PoacherB));
		TestFalse(TEXT("Third poacher is held out"), Rules->TryAcquirePlayerAttackSlot(PoacherC));
		TestEqual(TEXT("Exactly two slots are active"), Rules->GetActivePlayerAttackSlotCount(), 2);
		Rules->ReleasePlayerAttackSlot(PoacherA);
		TestTrue(TEXT("Released slot can be reassigned"), Rules->TryAcquirePlayerAttackSlot(PoacherC));
		TestEqual(TEXT("Slot count remains capped"), Rules->GetActivePlayerAttackSlotCount(), 2);
	}

	DestroyCombatTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPPlayerDownRoundFailureTest,
	"KrugerConclusion.PoachingPatrol.Combat.PlayerDownFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPPlayerDownRoundFailureTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCombatTestWorld(TEXT("PPPlayerDownAutomationWorld"));
	if (!TestNotNull(TEXT("Automation world created"), World))
	{
		return false;
	}

	UEnvironmentLevelSubsystem* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	APPPoacherCharacter* Poacher = World->SpawnActor<APPPoacherCharacter>();
	if (TestNotNull(TEXT("Rules exist"), Rules) && TestNotNull(TEXT("Poacher exists"), Poacher))
	{
		Rules->RegisterPoacher(Poacher);
		Rules->ReportPlayerDowned();
		TestTrue(TEXT("Player down ends the round"), Rules->HasRoundEnded());
		TestEqual(TEXT("Player down is a failure"), Rules->GetFinalRoundResult().Outcome, EPPRoundOutcome::Failure);
		const FPPRoundResult FirstResult = Rules->GetFinalRoundResult();
		Rules->ReportPlayerDowned();
		TestEqual(TEXT("Duplicate player down is idempotent"), Rules->GetFinalRoundResult().Outcome, FirstResult.Outcome);
	}

	DestroyCombatTestWorld(World);
	return true;
}

#endif
