#include "UI/PPUpgradeMenuWidget.h"

#include "UI/PPUIStyle.h"
#include "Data/PPGameFlowSubsystem.h"
#include "Kruger_ConclusionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor UpgradeInk(0.11f, 0.085f, 0.045f, 1.0f);
	const FLinearColor UpgradePaper(0.84f, 0.76f, 0.58f, 0.98f);
	const FLinearColor UpgradeGold(0.96f, 0.72f, 0.35f, 1.0f);
	const FLinearColor UpgradeCream(1.0f, 0.95f, 0.84f, 1.0f);
}

UPPUpgradeMenuWidget::UPPUpgradeMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> HealthFinder(TEXT("/Game/Poaching_Patrol/UI/UpgradeMenu_Assets/Health_Upgrade.Health_Upgrade"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MagazineFinder(TEXT("/Game/Poaching_Patrol/UI/UpgradeMenu_Assets/Magazine_Upgrade.Magazine_Upgrade"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> RangeFinder(TEXT("/Game/Poaching_Patrol/UI/UpgradeMenu_Assets/Range_Upgrade.Range_Upgrade"));
	if (HealthFinder.Succeeded()) HealthIcon = HealthFinder.Object;
	if (MagazineFinder.Succeeded()) MagazineIcon = MagazineFinder.Object;
	if (RangeFinder.Succeeded()) RangeIcon = RangeFinder.Object;
}

void UPPUpgradeMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	RefreshOffers();
}

void UPPUpgradeMenuWidget::Configure(EPPUpgradeContinueDestination InDestination)
{
	ContinueDestination = InDestination;
	RefreshOffers();
}

void UPPUpgradeMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	auto UpdateHover = [](FUpgradeCellWidgets& Cell)
	{
		if (!Cell.bActive || !Cell.Root || !Cell.Hover)
		{
			return;
		}
		// Whole-cell hover; keep Hidden (not Collapsed) so the BUY button does not shift.
		const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();
		const bool bHovered = Cell.Root->GetCachedGeometry().IsUnderLocation(CursorPos);
		Cell.Hover->SetVisibility(bHovered
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Hidden);
	};
	UpdateHover(LeftCell);
	UpdateHover(RightCell);
}

void UPPUpgradeMenuWidget::BuildWidgetTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("UpgradeCanvas"));
	WidgetTree->RootWidget = Canvas;

	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dimmer"));
	Dimmer->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.008f, 0.84f));
	if (UCanvasPanelSlot* DimmerSlot = Canvas->AddChildToCanvas(Dimmer))
	{
		DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		DimmerSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UpgradePanel"));
	Panel->SetBrush(PPUIStyle::PanelBrush(UpgradePaper));
	Panel->SetPadding(FMargin(28.0f));
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f));
		PanelSlot->SetSize(FVector2D(860.0f, 560.0f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeColumn"));
	Panel->SetContent(Column);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("UPGRADE MENU")));
	Title->SetFont(PPUIStyle::Font(TEXT("Bold"), 34));
	Title->SetColorAndOpacity(FSlateColor(UpgradeInk));
	Title->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	MoneyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Money"));
	MoneyText->SetFont(PPUIStyle::Font(TEXT("Bold"), 20));
	MoneyText->SetColorAndOpacity(FSlateColor(UpgradeGold));
	MoneyText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(MoneyText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	OfferRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("OfferRow"));
	if (UVerticalBoxSlot* OfferSlot = Column->AddChildToVerticalBox(OfferRow))
	{
		OfferSlot->SetHorizontalAlignment(HAlign_Center);
		OfferSlot->SetPadding(FMargin(0.0f, 8.0f));
	}

	LeftCell = BuildCell(OfferRow, TEXT("Left"));
	RightCell = BuildCell(OfferRow, TEXT("Right"));
	LeftCell.BuyButton->OnClicked.AddDynamic(this, &UPPUpgradeMenuWidget::HandleBuyLeftClicked);
	RightCell.BuyButton->OnClicked.AddDynamic(this, &UPPUpgradeMenuWidget::HandleBuyRightClicked);

	EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
	EmptyText->SetText(FText::FromString(TEXT("All upgrades bought")));
	EmptyText->SetFont(PPUIStyle::Font(TEXT("Bold"), 24));
	EmptyText->SetColorAndOpacity(FSlateColor(UpgradeInk));
	EmptyText->SetJustification(ETextJustify::Center);
	EmptyText->SetVisibility(ESlateVisibility::Collapsed);
	Column->AddChildToVerticalBox(EmptyText)->SetPadding(FMargin(0.0f, 24.0f));

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NavRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = Column->AddChildToVerticalBox(ButtonRow))
	{
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonRowSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
	}

	auto AddNavButton = [this, ButtonRow](const TCHAR* Name, const TCHAR* Label, void (UPPUpgradeMenuWidget::*Handler)())
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		PPUIStyle::OutlineButton(Button);
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>();
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetFont(PPUIStyle::Font(TEXT("Bold"), 18));
		LabelText->SetColorAndOpacity(FSlateColor(UpgradeCream));
		Button->SetContent(LabelText);
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(LabelText->Slot))
		{
			ContentSlot->SetPadding(FMargin(22.0f, 10.0f));
		}
		if (UHorizontalBoxSlot* NavSlot = ButtonRow->AddChildToHorizontalBox(Button))
		{
			NavSlot->SetPadding(FMargin(10.0f, 0.0f));
		}
		if (Handler == &UPPUpgradeMenuWidget::HandleContinueClicked)
		{
			Button->OnClicked.AddDynamic(this, &UPPUpgradeMenuWidget::HandleContinueClicked);
		}
		else
		{
			Button->OnClicked.AddDynamic(this, &UPPUpgradeMenuWidget::HandleMainMenuClicked);
		}
	};

	AddNavButton(TEXT("ContinueButton"), TEXT("CONTINUE"), &UPPUpgradeMenuWidget::HandleContinueClicked);
	AddNavButton(TEXT("MainMenuButton"), TEXT("MAIN MENU"), &UPPUpgradeMenuWidget::HandleMainMenuClicked);
}

UPPUpgradeMenuWidget::FUpgradeCellWidgets UPPUpgradeMenuWidget::BuildCell(UHorizontalBox* Parent, const TCHAR* NamePrefix)
{
	FUpgradeCellWidgets Cell;
	Cell.Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("%sRoot"), NamePrefix)));
	Cell.Root->SetBrush(PPUIStyle::PanelBrush(FLinearColor(0.16f, 0.13f, 0.07f, 0.95f)));
	Cell.Root->SetPadding(FMargin(16.0f));
	if (UHorizontalBoxSlot* CellSlot = Parent->AddChildToHorizontalBox(Cell.Root))
	{
		CellSlot->SetPadding(FMargin(12.0f, 0.0f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Cell.Root->SetContent(Column);

	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
	IconBox->SetWidthOverride(180.0f);
	IconBox->SetHeightOverride(180.0f);
	Column->AddChildToVerticalBox(IconBox)->SetHorizontalAlignment(HAlign_Center);

	Cell.Icon = WidgetTree->ConstructWidget<UImage>();
	Cell.Icon->SetDesiredSizeOverride(FVector2D(180.0f, 180.0f));
	IconBox->AddChild(Cell.Icon);

	Cell.Title = WidgetTree->ConstructWidget<UTextBlock>();
	Cell.Title->SetFont(PPUIStyle::Font(TEXT("Bold"), 18));
	Cell.Title->SetColorAndOpacity(FSlateColor(UpgradeCream));
	Cell.Title->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Cell.Title)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 4.0f));

	Cell.Cost = WidgetTree->ConstructWidget<UTextBlock>();
	Cell.Cost->SetFont(PPUIStyle::Font(TEXT("Bold"), 16));
	Cell.Cost->SetColorAndOpacity(FSlateColor(UpgradeGold));
	Cell.Cost->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(Cell.Cost)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	Cell.Hover = WidgetTree->ConstructWidget<UTextBlock>();
	Cell.Hover->SetFont(PPUIStyle::Font(TEXT("Regular"), 13));
	Cell.Hover->SetColorAndOpacity(FSlateColor(UpgradeCream));
	Cell.Hover->SetJustification(ETextJustify::Center);
	Cell.Hover->SetAutoWrapText(true);
	// Hidden reserves layout space so revealing hover text does not push BUY down.
	Cell.Hover->SetVisibility(ESlateVisibility::Hidden);
	if (UVerticalBoxSlot* HoverSlot = Column->AddChildToVerticalBox(Cell.Hover))
	{
		HoverSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		HoverSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	Cell.BuyButton = WidgetTree->ConstructWidget<UButton>();
	PPUIStyle::OutlineButton(Cell.BuyButton);
	UTextBlock* BuyLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BuyLabel->SetText(FText::FromString(TEXT("BUY")));
	BuyLabel->SetFont(PPUIStyle::Font(TEXT("Bold"), 16));
	BuyLabel->SetColorAndOpacity(FSlateColor(UpgradeCream));
	Cell.BuyButton->SetContent(BuyLabel);
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(BuyLabel->Slot))
	{
		ContentSlot->SetPadding(FMargin(18.0f, 8.0f));
	}
	Column->AddChildToVerticalBox(Cell.BuyButton)->SetHorizontalAlignment(HAlign_Center);
	return Cell;
}

void UPPUpgradeMenuWidget::RefreshOffers()
{
	UPPGameFlowSubsystem* Flow = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		Flow = GameInstance->GetSubsystem<UPPGameFlowSubsystem>();
	}

	if (MoneyText)
	{
		MoneyText->SetText(FText::FromString(FString::Printf(TEXT("MONEY: %d"), Flow ? Flow->GetMoney() : 0)));
	}

	TArray<EPPUpgradeType> Offers;
	if (Flow)
	{
		Flow->GetRandomOfferPair(Offers);
	}

	HideCell(LeftCell);
	HideCell(RightCell);

	if (Offers.Num() == 0)
	{
		if (OfferRow) OfferRow->SetVisibility(ESlateVisibility::Collapsed);
		if (EmptyText) EmptyText->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	if (OfferRow) OfferRow->SetVisibility(ESlateVisibility::Visible);
	if (EmptyText) EmptyText->SetVisibility(ESlateVisibility::Collapsed);

	PopulateCell(LeftCell, Offers[0]);
	if (Offers.Num() > 1)
	{
		PopulateCell(RightCell, Offers[1]);
	}
}

void UPPUpgradeMenuWidget::PopulateCell(FUpgradeCellWidgets& Cell, EPPUpgradeType Type)
{
	UPPGameFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPPGameFlowSubsystem>() : nullptr;
	Cell.Type = Type;
	Cell.bActive = true;
	if (Cell.Root) Cell.Root->SetVisibility(ESlateVisibility::Visible);
	if (Cell.Icon)
	{
		if (UTexture2D* Icon = GetIconForType(Type))
		{
			Cell.Icon->SetBrushFromTexture(Icon, true);
		}
	}
	if (Cell.Title) Cell.Title->SetText(GetTitleForType(Type));
	if (Cell.Cost)
	{
		Cell.Cost->SetText(FText::FromString(FString::Printf(TEXT("COST %d"), Flow ? Flow->GetUpgradeCost(Type) : 100)));
	}
	if (Cell.Hover)
	{
		Cell.Hover->SetText(GetHoverForType(Type));
		Cell.Hover->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Cell.BuyButton)
	{
		const bool bCanBuy = Flow && Flow->CanPurchaseUpgrade(Type) && Flow->GetMoney() >= Flow->GetUpgradeCost(Type);
		Cell.BuyButton->SetIsEnabled(bCanBuy);
	}
}

void UPPUpgradeMenuWidget::HideCell(FUpgradeCellWidgets& Cell)
{
	Cell.bActive = false;
	if (Cell.Root) Cell.Root->SetVisibility(ESlateVisibility::Collapsed);
	if (Cell.Hover) Cell.Hover->SetVisibility(ESlateVisibility::Hidden);
}

void UPPUpgradeMenuWidget::HandleBuy(FUpgradeCellWidgets& Cell)
{
	if (!Cell.bActive)
	{
		return;
	}

	UPPGameFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPPGameFlowSubsystem>() : nullptr;
	if (!Flow || !Flow->PurchaseUpgrade(Cell.Type))
	{
		RefreshOffers();
		return;
	}

	RefreshOffers();
}

void UPPUpgradeMenuWidget::HandleBuyLeftClicked() { HandleBuy(LeftCell); }
void UPPUpgradeMenuWidget::HandleBuyRightClicked() { HandleBuy(RightCell); }

void UPPUpgradeMenuWidget::HandleContinueClicked()
{
	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>())
	{
		Controller->ContinueFromUpgradeMenu();
	}
}

void UPPUpgradeMenuWidget::HandleMainMenuClicked()
{
	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>())
	{
		Controller->ReturnToPoachingPatrolMenu();
	}
}

UTexture2D* UPPUpgradeMenuWidget::GetIconForType(EPPUpgradeType Type) const
{
	switch (Type)
	{
	case EPPUpgradeType::Health: return HealthIcon;
	case EPPUpgradeType::Magazine: return MagazineIcon;
	case EPPUpgradeType::Range: return RangeIcon;
	default: return nullptr;
	}
}

FText UPPUpgradeMenuWidget::GetTitleForType(EPPUpgradeType Type)
{
	switch (Type)
	{
	case EPPUpgradeType::Health: return FText::FromString(TEXT("HEALTH UPGRADE"));
	case EPPUpgradeType::Magazine: return FText::FromString(TEXT("MAGAZINE UPGRADE"));
	case EPPUpgradeType::Range: return FText::FromString(TEXT("RANGE UPGRADE"));
	default: return FText::GetEmpty();
	}
}

FText UPPUpgradeMenuWidget::GetHoverForType(EPPUpgradeType Type)
{
	switch (Type)
	{
	case EPPUpgradeType::Health: return FText::FromString(TEXT("increase player health"));
	case EPPUpgradeType::Magazine: return FText::FromString(TEXT("Get an extra magazine(Reload your weapon with 'R')."));
	case EPPUpgradeType::Range: return FText::FromString(TEXT("Double pepper spray gun range."));
	default: return FText::GetEmpty();
	}
}
