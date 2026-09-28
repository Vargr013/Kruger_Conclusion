#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PPGameTypes.h"
#include "PPUpgradeMenuWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

UCLASS()
class KRUGER_CONCLUSION_API UPPUpgradeMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPPUpgradeMenuWidget(const FObjectInitializer& ObjectInitializer);

	void Configure(EPPUpgradeContinueDestination InDestination);
	void RefreshOffers();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleBuyLeftClicked();

	UFUNCTION()
	void HandleBuyRightClicked();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

private:
	struct FUpgradeCellWidgets
	{
		TObjectPtr<UBorder> Root = nullptr;
		TObjectPtr<UImage> Icon = nullptr;
		TObjectPtr<UTextBlock> Title = nullptr;
		TObjectPtr<UTextBlock> Cost = nullptr;
		TObjectPtr<UTextBlock> Hover = nullptr;
		TObjectPtr<UButton> BuyButton = nullptr;
		EPPUpgradeType Type = EPPUpgradeType::Health;
		bool bActive = false;
	};

	void BuildWidgetTree();
	FUpgradeCellWidgets BuildCell(UHorizontalBox* Parent, const TCHAR* NamePrefix);
	void PopulateCell(FUpgradeCellWidgets& Cell, EPPUpgradeType Type);
	void HideCell(FUpgradeCellWidgets& Cell);
	void HandleBuy(FUpgradeCellWidgets& Cell);
	UTexture2D* GetIconForType(EPPUpgradeType Type) const;
	static FText GetTitleForType(EPPUpgradeType Type);
	static FText GetHoverForType(EPPUpgradeType Type);

	UPROPERTY()
	TObjectPtr<UTextBlock> MoneyText;

	UPROPERTY()
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> OfferRow;

	UPROPERTY()
	TObjectPtr<UTexture2D> HealthIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> MagazineIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> RangeIcon;

	FUpgradeCellWidgets LeftCell;
	FUpgradeCellWidgets RightCell;
	EPPUpgradeContinueDestination ContinueDestination = EPPUpgradeContinueDestination::NextPatrolDay;
};
