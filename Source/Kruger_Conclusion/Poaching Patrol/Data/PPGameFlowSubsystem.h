#pragma once

#include "CoreMinimal.h"
#include "Data/PPGameTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PPGameFlowSubsystem.generated.h"

class ARangerCharacter;

UCLASS()
class KRUGER_CONCLUSION_API UPPGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxHealthUpgrades = 4;
	static constexpr int32 MaxMagazineUpgrades = 3;
	static constexpr int32 BaseUpgradeCost = 100;
	static constexpr float HealthBonusPerUpgrade = 50.0f;

	void RequestReplayBypass() { bBypassOpeningMenuOnce = true; }
	void ClearReplayBypass() { bBypassOpeningMenuOnce = false; }
	bool ConsumeReplayBypass();
	void RequestPatrolMode(bool bTutorialMode);
	void ClearRequestedPatrolMode() { bHasRequestedPatrolMode = false; }
	bool ConsumeRequestedPatrolMode(bool& bOutTutorialMode);

	UFUNCTION(BlueprintPure, Category = "Poaching Patrol|Money")
	int32 GetMoney() const { return Money; }

	UFUNCTION(BlueprintCallable, Category = "Poaching Patrol|Money")
	void AddMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Poaching Patrol|Money")
	void SetMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Poaching Patrol|Money")
	bool TrySpendMoney(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Poaching Patrol|Money")
	int32 GetIncomePerAnimalAlive() const { return IncomePerAnimalAlive; }

	UFUNCTION(BlueprintPure, Category = "Poaching Patrol|Upgrades")
	int32 GetHealthUpgradeCount() const { return HealthUpgradeCount; }

	UFUNCTION(BlueprintPure, Category = "Poaching Patrol|Upgrades")
	int32 GetMagazineUpgradeCount() const { return MagazineUpgradeCount; }

	UFUNCTION(BlueprintPure, Category = "Poaching Patrol|Upgrades")
	bool HasRangeUpgrade() const { return bRangeUpgrade; }

	UFUNCTION(BlueprintPure, Category = "Poaching Patrol|Upgrades")
	int32 GetUpgradeCost(EPPUpgradeType Type) const;

	UFUNCTION(BlueprintPure, Category = "Poaching Patrol|Upgrades")
	bool CanPurchaseUpgrade(EPPUpgradeType Type) const;

	UFUNCTION(BlueprintCallable, Category = "Poaching Patrol|Upgrades")
	bool PurchaseUpgrade(EPPUpgradeType Type);

	UFUNCTION(BlueprintCallable, Category = "Poaching Patrol|Upgrades")
	void GetRandomOfferPair(TArray<EPPUpgradeType>& OutOffers) const;

	UFUNCTION(BlueprintCallable, Category = "Poaching Patrol|Upgrades")
	void ApplyOwnedUpgrades(ARangerCharacter* Ranger) const;

private:
	int32 GetPurchasedCount(EPPUpgradeType Type) const;
	void CollectAvailableUpgrades(TArray<EPPUpgradeType>& OutAvailable) const;

	bool bBypassOpeningMenuOnce = false;
	bool bHasRequestedPatrolMode = false;
	bool bRequestedTutorialMode = false;

	UPROPERTY(VisibleAnywhere, Category = "Poaching Patrol|Money")
	int32 Money = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Poaching Patrol|Money", meta = (ClampMin = "0"))
	int32 IncomePerAnimalAlive = 10;

	UPROPERTY(VisibleAnywhere, Category = "Poaching Patrol|Upgrades")
	int32 HealthUpgradeCount = 0;

	UPROPERTY(VisibleAnywhere, Category = "Poaching Patrol|Upgrades")
	int32 MagazineUpgradeCount = 0;

	UPROPERTY(VisibleAnywhere, Category = "Poaching Patrol|Upgrades")
	bool bRangeUpgrade = false;
};
