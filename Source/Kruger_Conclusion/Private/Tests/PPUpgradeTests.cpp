#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "BaseGun.h"
#include "Characters/ARangerCharacter.h"
#include "Data/PPGameFlowSubsystem.h"
#include "Data/PPGameTypes.h"
#include "Data/PPHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPUpgradePurchaseRulesTest,
	"KrugerConclusion.PoachingPatrol.Upgrades.PurchaseRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPUpgradePurchaseRulesTest::RunTest(const FString& Parameters)
{
	UPPGameFlowSubsystem* Flow = NewObject<UPPGameFlowSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("Game flow object exists"), Flow))
	{
		return false;
	}

	Flow->SetMoney(1000);
	TestEqual(TEXT("First health upgrade costs 100"), Flow->GetUpgradeCost(EPPUpgradeType::Health), 100);
	TestTrue(TEXT("First health upgrade purchases"), Flow->PurchaseUpgrade(EPPUpgradeType::Health));
	TestEqual(TEXT("Health stack is one"), Flow->GetHealthUpgradeCount(), 1);
	TestEqual(TEXT("Second health upgrade costs 200"), Flow->GetUpgradeCost(EPPUpgradeType::Health), 200);
	TestTrue(TEXT("Second health upgrade purchases"), Flow->PurchaseUpgrade(EPPUpgradeType::Health));
	TestTrue(TEXT("Third health upgrade purchases"), Flow->PurchaseUpgrade(EPPUpgradeType::Health));
	TestTrue(TEXT("Fourth health upgrade purchases"), Flow->PurchaseUpgrade(EPPUpgradeType::Health));
	TestEqual(TEXT("Health stack caps at four"), Flow->GetHealthUpgradeCount(), 4);
	TestFalse(TEXT("Fifth health upgrade is rejected"), Flow->PurchaseUpgrade(EPPUpgradeType::Health));

	TestTrue(TEXT("First magazine upgrade purchases"), Flow->PurchaseUpgrade(EPPUpgradeType::Magazine));
	TestTrue(TEXT("Second magazine upgrade purchases"), Flow->PurchaseUpgrade(EPPUpgradeType::Magazine));
	TestTrue(TEXT("Third magazine upgrade purchases"), Flow->PurchaseUpgrade(EPPUpgradeType::Magazine));
	TestEqual(TEXT("Magazine stack caps at three"), Flow->GetMagazineUpgradeCount(), 3);
	TestFalse(TEXT("Fourth magazine upgrade is rejected"), Flow->PurchaseUpgrade(EPPUpgradeType::Magazine));

	TestEqual(TEXT("Range upgrade costs 100"), Flow->GetUpgradeCost(EPPUpgradeType::Range), 100);
	TestTrue(TEXT("Range upgrade purchases once"), Flow->PurchaseUpgrade(EPPUpgradeType::Range));
	TestFalse(TEXT("Range upgrade cannot be bought twice"), Flow->PurchaseUpgrade(EPPUpgradeType::Range));

	TArray<EPPUpgradeType> Offers;
	Flow->GetRandomOfferPair(Offers);
	TestEqual(TEXT("No offers remain when all upgrades are maxed"), Offers.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPUpgradeApplyEffectsTest,
	"KrugerConclusion.PoachingPatrol.Upgrades.ApplyEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPUpgradeApplyEffectsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("PPUpgradeApplyWorld"));
	if (!TestNotNull(TEXT("Automation world created"), World))
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	UPPGameFlowSubsystem* Flow = NewObject<UPPGameFlowSubsystem>(GetTransientPackage());
	ARangerCharacter* Ranger = World->SpawnActor<ARangerCharacter>();
	ABaseGun* Gun = World->SpawnActor<ABaseGun>();
	if (!TestNotNull(TEXT("Game flow exists"), Flow)
		|| !TestNotNull(TEXT("Ranger exists"), Ranger)
		|| !TestNotNull(TEXT("Gun exists"), Gun))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}

	Ranger->SetCurrentGun(Gun);
	Flow->SetMoney(2000);
	Flow->PurchaseUpgrade(EPPUpgradeType::Health);
	Flow->PurchaseUpgrade(EPPUpgradeType::Health);
	Flow->PurchaseUpgrade(EPPUpgradeType::Magazine);
	Flow->PurchaseUpgrade(EPPUpgradeType::Magazine);
	Flow->PurchaseUpgrade(EPPUpgradeType::Range);
	Flow->ApplyOwnedUpgrades(Ranger);

	UPPHealthComponent* Health = Ranger->GetHealthComponent();
	if (TestNotNull(TEXT("Health component exists"), Health))
	{
		TestEqual(TEXT("Two health upgrades reach 200 max"), Health->GetMaxHealth(), 200.0f);
	}

	TestEqual(TEXT("Two magazine upgrades grant two spare mags"), Gun->GetSpareMagazineCapacity(), 2);
	TestEqual(TEXT("Spare mags start full after apply"), Gun->GetSpareMagazinesRemaining(), 2);
	TestEqual(TEXT("Range upgrade doubles spray range"), Gun->SprayRange, 1000.0f);

	Gun->Shoot();
	TestTrue(TEXT("Spare magazine reloads when ammo is spent"), Gun->TryUseSpareMagazine());
	TestEqual(TEXT("One spare mag remains after reload"), Gun->GetSpareMagazinesRemaining(), 1);
	TestFalse(TEXT("Reload is refused when magazine is full"), Gun->TryUseSpareMagazine());

	Ranger->Resupply();
	TestEqual(TEXT("Resupply restores spare magazines to capacity"), Gun->GetSpareMagazinesRemaining(), 2);

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
