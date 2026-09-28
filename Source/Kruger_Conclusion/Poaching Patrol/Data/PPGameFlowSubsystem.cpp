#include "Data/PPGameFlowSubsystem.h"

#include "BaseGun.h"
#include "Characters/ARangerCharacter.h"

bool UPPGameFlowSubsystem::ConsumeReplayBypass()
{
	const bool bShouldBypass = bBypassOpeningMenuOnce;
	bBypassOpeningMenuOnce = false;
	return bShouldBypass;
}

void UPPGameFlowSubsystem::RequestPatrolMode(bool bTutorialMode)
{
	bHasRequestedPatrolMode = true;
	bRequestedTutorialMode = bTutorialMode;
}

bool UPPGameFlowSubsystem::ConsumeRequestedPatrolMode(bool& bOutTutorialMode)
{
	if (!bHasRequestedPatrolMode)
	{
		return false;
	}

	bOutTutorialMode = bRequestedTutorialMode;
	bHasRequestedPatrolMode = false;
	return true;
}

void UPPGameFlowSubsystem::AddMoney(int32 Amount)
{
	if (Amount == 0)
	{
		return;
	}

	Money = FMath::Max(0, Money + Amount);
}

void UPPGameFlowSubsystem::SetMoney(int32 Amount)
{
	Money = FMath::Max(0, Amount);
}

bool UPPGameFlowSubsystem::TrySpendMoney(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}
	if (Money < Amount)
	{
		return false;
	}

	Money -= Amount;
	return true;
}

int32 UPPGameFlowSubsystem::GetPurchasedCount(EPPUpgradeType Type) const
{
	switch (Type)
	{
	case EPPUpgradeType::Health: return HealthUpgradeCount;
	case EPPUpgradeType::Magazine: return MagazineUpgradeCount;
	case EPPUpgradeType::Range: return bRangeUpgrade ? 1 : 0;
	default: return 0;
	}
}

int32 UPPGameFlowSubsystem::GetUpgradeCost(EPPUpgradeType Type) const
{
	if (Type == EPPUpgradeType::Range)
	{
		return BaseUpgradeCost;
	}

	const int32 Purchased = GetPurchasedCount(Type);
	return BaseUpgradeCost << Purchased;
}

bool UPPGameFlowSubsystem::CanPurchaseUpgrade(EPPUpgradeType Type) const
{
	switch (Type)
	{
	case EPPUpgradeType::Health:
		return HealthUpgradeCount < MaxHealthUpgrades;
	case EPPUpgradeType::Magazine:
		return MagazineUpgradeCount < MaxMagazineUpgrades;
	case EPPUpgradeType::Range:
		return !bRangeUpgrade;
	default:
		return false;
	}
}

bool UPPGameFlowSubsystem::PurchaseUpgrade(EPPUpgradeType Type)
{
	if (!CanPurchaseUpgrade(Type))
	{
		return false;
	}

	const int32 Cost = GetUpgradeCost(Type);
	if (!TrySpendMoney(Cost))
	{
		return false;
	}

	switch (Type)
	{
	case EPPUpgradeType::Health:
		++HealthUpgradeCount;
		break;
	case EPPUpgradeType::Magazine:
		++MagazineUpgradeCount;
		break;
	case EPPUpgradeType::Range:
		bRangeUpgrade = true;
		break;
	default:
		AddMoney(Cost);
		return false;
	}

	return true;
}

void UPPGameFlowSubsystem::CollectAvailableUpgrades(TArray<EPPUpgradeType>& OutAvailable) const
{
	OutAvailable.Reset();
	for (EPPUpgradeType Type : { EPPUpgradeType::Health, EPPUpgradeType::Magazine, EPPUpgradeType::Range })
	{
		if (CanPurchaseUpgrade(Type))
		{
			OutAvailable.Add(Type);
		}
	}
}

void UPPGameFlowSubsystem::GetRandomOfferPair(TArray<EPPUpgradeType>& OutOffers) const
{
	TArray<EPPUpgradeType> Available;
	CollectAvailableUpgrades(Available);

	for (int32 Index = Available.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Available.Swap(Index, SwapIndex);
	}

	OutOffers.Reset();
	const int32 Count = FMath::Min(2, Available.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		OutOffers.Add(Available[Index]);
	}
}

void UPPGameFlowSubsystem::ApplyOwnedUpgrades(ARangerCharacter* Ranger) const
{
	if (!IsValid(Ranger))
	{
		return;
	}

	if (UPPHealthComponent* Health = Ranger->GetHealthComponent())
	{
		const float Bonus = HealthBonusPerUpgrade * static_cast<float>(HealthUpgradeCount);
		if (Bonus > 0.0f)
		{
			Ranger->IncreasePlayerMaxHealth(Bonus);
		}
	}

	if (ABaseGun* Gun = Ranger->GetCurrentGun())
	{
		Gun->SetSpareMagazineCapacity(MagazineUpgradeCount);
		Gun->RestoreSpareMagazines();
		if (bRangeUpgrade)
		{
			Gun->ApplyRangeUpgrade();
		}
	}
}
