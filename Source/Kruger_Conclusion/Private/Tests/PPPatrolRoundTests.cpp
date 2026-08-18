#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Data/PPHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnvironmentLevelSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPPatrolRequiredArrestsTest,
	"KrugerConclusion.PoachingPatrol.Round.RequiredArrests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPPatrolRequiredArrestsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("No poachers requires no arrests"), UEnvironmentLevelSubsystem::CalculateRequiredArrests(0, 0.5f), 0);
	TestEqual(TEXT("One poacher rounds up"), UEnvironmentLevelSubsystem::CalculateRequiredArrests(1, 0.5f), 1);
	TestEqual(TEXT("Three poachers rounds up"), UEnvironmentLevelSubsystem::CalculateRequiredArrests(3, 0.5f), 2);
	TestEqual(TEXT("Four poachers remains exact"), UEnvironmentLevelSubsystem::CalculateRequiredArrests(4, 0.5f), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPPatrolRoundStateTest,
	"KrugerConclusion.PoachingPatrol.Round.StateAndIdempotency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPPatrolRoundStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("PPPatrolRoundAutomationWorld"));
	if (!TestNotNull(TEXT("Automation world created"), World))
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	UEnvironmentLevelSubsystem* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	APPPoacherCharacter* PoacherA = World->SpawnActor<APPPoacherCharacter>();
	APPPoacherCharacter* PoacherB = World->SpawnActor<APPPoacherCharacter>();
	APPPoacherCharacter* PoacherC = World->SpawnActor<APPPoacherCharacter>();
	APPAnimalCharacter* AnimalA = World->SpawnActor<APPAnimalCharacter>();
	APPAnimalCharacter* AnimalB = World->SpawnActor<APPAnimalCharacter>();

	if (TestNotNull(TEXT("Rules subsystem exists"), Rules)
		&& TestNotNull(TEXT("First poacher exists"), PoacherA)
		&& TestNotNull(TEXT("Second poacher exists"), PoacherB)
		&& TestNotNull(TEXT("Third poacher exists"), PoacherC)
		&& TestNotNull(TEXT("First animal exists"), AnimalA)
		&& TestNotNull(TEXT("Second animal exists"), AnimalB))
	{
		Rules->RegisterPoacher(PoacherA);
		Rules->RegisterPoacher(PoacherA);
		Rules->RegisterPoacher(PoacherB);
		Rules->RegisterPoacher(PoacherC);
		Rules->RegisterAnimal(AnimalA);
		Rules->RegisterAnimal(AnimalA);
		Rules->RegisterAnimal(AnimalB);

		FPPRoundSnapshot Snapshot = Rules->GetRoundSnapshot();
		TestEqual(TEXT("Duplicate registration is ignored"), Snapshot.TotalPoachers, 3);
		TestEqual(TEXT("Duplicate animal registration is ignored"), Snapshot.TotalAnimals, 2);
		TestEqual(TEXT("All registered poachers begin active on the minimap"), Rules->GetActivePoachers().Num(), 3);
		TestEqual(TEXT("All registered animals begin living on the minimap"), Rules->GetLivingAnimals().Num(), 2);

		for (const EPPPoacherState ActiveState : {
			EPPPoacherState::DisguisedRoaming,
			EPPPoacherState::Alert,
			EPPPoacherState::Fleeing,
			EPPPoacherState::EngagingPlayer,
			EPPPoacherState::Subdued,
			EPPPoacherState::Captured,
			EPPPoacherState::FollowingPlayer})
		{
			PoacherB->SetPoacherState(ActiveState);
			TestTrue(TEXT("Every gameplay-active poacher state remains in the minimap query"), Rules->GetActivePoachers().Contains(PoacherB));
		}
		PoacherB->SetPoacherState(EPPPoacherState::Arrested);
		TestFalse(TEXT("Arrested state is filtered immediately"), Rules->GetActivePoachers().Contains(PoacherB));
		PoacherB->SetPoacherState(EPPPoacherState::Escaped);
		TestFalse(TEXT("Escaped state is filtered immediately"), Rules->GetActivePoachers().Contains(PoacherB));
		PoacherB->SetPoacherState(EPPPoacherState::DisguisedRoaming);

		if (UPPHealthComponent* AnimalHealth = AnimalA->FindComponentByClass<UPPHealthComponent>())
		{
			AnimalHealth->ApplyDamage(AnimalHealth->GetMaxHealth());
			TestFalse(TEXT("Dead animals are filtered immediately"), Rules->GetLivingAnimals().Contains(AnimalA));
		}

		Rules->ReportAnimalPoached(AnimalA);
		Rules->ReportAnimalPoached(AnimalA);
		Snapshot = Rules->GetRoundSnapshot();
		TestEqual(TEXT("Duplicate animal report is ignored"), Snapshot.AnimalsPoached, 1);
		TestEqual(TEXT("Animals saved remains consistent"), Snapshot.AnimalsAlive, 1);
		TestEqual(TEXT("Poached animals are removed from the minimap query"), Rules->GetLivingAnimals().Num(), 1);

		Rules->ReportPoacherArrested(PoacherA);
		Rules->ReportPoacherArrested(PoacherA);
		TestFalse(TEXT("Round remains active with unresolved poachers"), Rules->HasRoundEnded());
		TestEqual(TEXT("Duplicate arrest is ignored"), Rules->GetRoundSnapshot().PoachersArrested, 1);
		TestEqual(TEXT("Arrested poachers are removed from the minimap query"), Rules->GetActivePoachers().Num(), 2);

		// A temporary escort break does not call a permanent report method.
		TestEqual(TEXT("Temporary escort break leaves permanent count unchanged"), Rules->GetRoundSnapshot().PoachersPermanentlyEscaped, 0);

		Rules->ReportPoacherArrested(PoacherB);
		TestFalse(TEXT("One active poacher still prevents completion"), Rules->HasRoundEnded());
		Rules->ReportPoacherPermanentlyEscaped(PoacherC);
		TestTrue(TEXT("All accounted poachers complete the round"), Rules->HasRoundEnded());
		TestEqual(TEXT("Two of three arrests meets the rounded quota"), Rules->GetFinalRoundResult().Outcome, EPPRoundOutcome::Success);

		const FPPRoundResult FirstResult = Rules->GetFinalRoundResult();
		Rules->ReportPoacherPermanentlyEscaped(PoacherC);
		TestEqual(TEXT("Final result remains immutable"), Rules->GetFinalRoundResult().Snapshot.PoachersPermanentlyEscaped, FirstResult.Snapshot.PoachersPermanentlyEscaped);
	}

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
