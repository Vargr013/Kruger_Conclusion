#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/PPRestPoint.h"
#include "BaseGun.h"
#include "Characters/ARangerCharacter.h"
#include "Data/PPHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPRestPointHoldResupplyTest,
	"KrugerConclusion.PoachingPatrol.RestPoint.HoldResupply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPRestPointHoldResupplyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("PPRestPointAutomationWorld"));
	if (!TestNotNull(TEXT("Automation world created"), World))
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	APPRestPoint* RestPoint = World->SpawnActor<APPRestPoint>();
	ARangerCharacter* Ranger = World->SpawnActor<ARangerCharacter>();
	ABaseGun* Gun = World->SpawnActor<ABaseGun>();

	if (!TestNotNull(TEXT("Rest point exists"), RestPoint)
		|| !TestNotNull(TEXT("Ranger exists"), Ranger)
		|| !TestNotNull(TEXT("Gun exists"), Gun))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}

	Ranger->SetCurrentGun(Gun);
	for (int32 ShotIndex = 0; ShotIndex < Gun->GetMaxUses(); ++ShotIndex)
	{
		Gun->Shoot();
	}
	TestEqual(TEXT("Gun is empty before restock"), Gun->GetRemainingUses(), 0);

	UPPHealthComponent* Health = Ranger->GetHealthComponent();
	if (TestNotNull(TEXT("Ranger has a health component"), Health))
	{
		Health->ApplyDamage(40.0f);
		TestEqual(TEXT("Health is reduced before restock"), Health->GetCurrentHealth(), Health->GetMaxHealth() - 40.0f);
	}

	TestFalse(TEXT("Ranger is not in range before overlap"), RestPoint->IsRangerInRange());
	RestPoint->SetOverlappingRanger(Ranger);
	TestTrue(TEXT("Ranger is in range after overlap"), RestPoint->IsRangerInRangeFor(Ranger));
	TestEqual(TEXT("Hold progress starts at zero"), RestPoint->GetHoldProgress(), 0.0f);

	RestPoint->AdvanceHold(1.0f, true);
	TestTrue(TEXT("A partial hold does not complete restock"), RestPoint->GetHoldProgress() > 0.0f && RestPoint->GetHoldProgress() < 1.0f);
	if (Health)
	{
		TestEqual(TEXT("Partial hold does not restore health"), Health->GetCurrentHealth(), Health->GetMaxHealth() - 40.0f);
	}
	TestEqual(TEXT("Partial hold does not reload the gun"), Gun->GetRemainingUses(), 0);

	RestPoint->AdvanceHold(1.0f, false);
	TestEqual(TEXT("Releasing E cancels hold progress"), RestPoint->GetHoldProgress(), 0.0f);

	RestPoint->AdvanceHold(2.0f, true);
	TestEqual(TEXT("A full hold fills the progress bar"), RestPoint->GetHoldProgress(), 1.0f);
	if (Health)
	{
		TestEqual(TEXT("A full hold restores health"), Health->GetCurrentHealth(), Health->GetMaxHealth());
	}
	TestEqual(TEXT("A full hold reloads the gun"), Gun->GetRemainingUses(), Gun->GetMaxUses());

	if (Health)
	{
		Health->ApplyDamage(25.0f);
	}
	for (int32 ShotIndex = 0; ShotIndex < 3; ++ShotIndex)
	{
		Gun->Shoot();
	}
	RestPoint->AdvanceHold(2.0f, true);
	if (Health)
	{
		TestEqual(TEXT("Holding after restock without releasing does not restock again"), Health->GetCurrentHealth(), Health->GetMaxHealth() - 25.0f);
	}

	RestPoint->AdvanceHold(0.1f, false);
	RestPoint->AdvanceHold(2.0f, true);
	if (Health)
	{
		TestEqual(TEXT("A second hold after release restocks again"), Health->GetCurrentHealth(), Health->GetMaxHealth());
	}
	TestEqual(TEXT("A second hold reloads the gun again"), Gun->GetRemainingUses(), Gun->GetMaxUses());

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
