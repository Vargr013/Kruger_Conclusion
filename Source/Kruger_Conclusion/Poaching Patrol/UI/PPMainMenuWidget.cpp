#include "UI/PPMainMenuWidget.h"
#include "UI/PPUIStyle.h"
#include "UI/PPOptionsMenuWidget.h"

#include "Kruger_ConclusionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float MainMenuUiScale = 1.5f;

	UButton* AddMainMenuButton(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, const FText& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
		PPUIStyle::OutlineButton(Button);
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Text->SetFont(PPUIStyle::Font(TEXT("Bold"), 24));
		Button->AddChild(Text);
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Text->Slot))
		{
			ContentSlot->SetPadding(FMargin(18.0f, 10.0f));
		}
		Box->AddChildToVerticalBox(Button)->SetPadding(FMargin(10.0f, 8.0f));
		return Button;
	}
}

UPPMainMenuWidget::UPPMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> BackgroundFinder(
		TEXT("/Game/Poaching_Patrol/UI/MainMenu_Assets/Main_Menu.Main_Menu"));
	if (BackgroundFinder.Succeeded())
	{
		MainMenuTexture = BackgroundFinder.Object;
	}
}

void UPPMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MainMenuCanvas"));
		WidgetTree->RootWidget = Canvas;

		UScaleBox* BackgroundScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("BackgroundScale"));
		BackgroundScale->SetStretch(EStretch::ScaleToFill);
		if (UCanvasPanelSlot* ScaleSlot = Canvas->AddChildToCanvas(BackgroundScale))
		{
			ScaleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			ScaleSlot->SetOffsets(FMargin(0.0f));
		}

		const float TextureWidth = MainMenuTexture ? static_cast<float>(MainMenuTexture->GetSizeX()) : 1672.0f;
		const float TextureHeight = MainMenuTexture ? static_cast<float>(MainMenuTexture->GetSizeY()) : 941.0f;

		USizeBox* BackgroundSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackgroundSize"));
		BackgroundSize->SetWidthOverride(TextureWidth);
		BackgroundSize->SetHeightOverride(TextureHeight);
		BackgroundScale->AddChild(BackgroundSize);

		BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainMenuBackground"));
		if (MainMenuTexture)
		{
			BackgroundImage->SetBrushFromTexture(MainMenuTexture, true);
		}
		BackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		BackgroundSize->AddChild(BackgroundImage);

		// Soft vignette so buttons stay readable without covering the title card art.
		ButtonScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ButtonScale"));
		ButtonScale->SetStretch(EStretch::UserSpecified);
		ButtonScale->SetUserSpecifiedScale(MainMenuUiScale);
		if (UCanvasPanelSlot* ScaleSlot = Canvas->AddChildToCanvas(ButtonScale))
		{
			ScaleSlot->SetAnchors(FAnchors(0.5f, 0.52f));
			ScaleSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			ScaleSlot->SetAutoSize(true);
		}

		ButtonShade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ButtonShade"));
		ButtonShade->SetBrushColor(FLinearColor(0.02f, 0.015f, 0.008f, 0.42f));
		ButtonShade->SetPadding(FMargin(28.0f, 18.0f));
		ButtonScale->AddChild(ButtonShade);

		RootMenu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuButtons"));
		ButtonShade->SetContent(RootMenu);

		StartButton = AddMainMenuButton(WidgetTree, RootMenu, TEXT("Start"), NSLOCTEXT("PoachingPatrol", "Start", "Start"));
		OptionsButton = AddMainMenuButton(WidgetTree, RootMenu, TEXT("Options"), NSLOCTEXT("PoachingPatrol", "Options", "Options"));
		QuitButton = AddMainMenuButton(WidgetTree, RootMenu, TEXT("Quit"), NSLOCTEXT("PoachingPatrol", "Quit", "Quit Game"));

		OptionsScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("OptionsScale"));
		OptionsScale->SetStretch(EStretch::UserSpecified);
		OptionsScale->SetUserSpecifiedScale(MainMenuUiScale);
		if (UCanvasPanelSlot* OptionsScaleSlot = Canvas->AddChildToCanvas(OptionsScale))
		{
			OptionsScaleSlot->SetAnchors(FAnchors(0.5f));
			OptionsScaleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			OptionsScaleSlot->SetAutoSize(true);
		}

		OptionsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OptionsPanel"));
		OptionsPanel->SetBrush(PPUIStyle::PanelBrush(FLinearColor(0.11f, 0.085f, 0.045f, 0.94f)));
		OptionsPanel->SetPadding(FMargin(20.0f, 16.0f));
		OptionsScale->AddChild(OptionsPanel);

		OptionsMenu = WidgetTree->ConstructWidget<UPPOptionsMenuWidget>(UPPOptionsMenuWidget::StaticClass(), TEXT("MainMenuOptions"));
		OptionsPanel->SetContent(OptionsMenu);
		OptionsScale->SetVisibility(ESlateVisibility::Collapsed);
	}

	StartButton->OnClicked.AddUniqueDynamic(this, &UPPMainMenuWidget::HandleStartClicked);
	OptionsButton->OnClicked.AddUniqueDynamic(this, &UPPMainMenuWidget::HandleOptionsClicked);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UPPMainMenuWidget::HandleQuitClicked);
	if (OptionsMenu)
	{
		OptionsMenu->OnBackRequested.AddUniqueDynamic(this, &UPPMainMenuWidget::HandleOptionsBack);
	}
	ShowRootMenu();
}

void UPPMainMenuWidget::ShowRootMenu()
{
	if (ButtonScale) ButtonScale->SetVisibility(ESlateVisibility::Visible);
	if (RootMenu) RootMenu->SetVisibility(ESlateVisibility::Visible);
	if (OptionsScale) OptionsScale->SetVisibility(ESlateVisibility::Collapsed);
	if (StartButton) StartButton->SetKeyboardFocus();
}

void UPPMainMenuWidget::ShowOptionsMenu()
{
	if (ButtonScale) ButtonScale->SetVisibility(ESlateVisibility::Collapsed);
	if (RootMenu) RootMenu->SetVisibility(ESlateVisibility::Collapsed);
	if (OptionsScale) OptionsScale->SetVisibility(ESlateVisibility::Visible);
	if (OptionsMenu) OptionsMenu->SetKeyboardFocus();
}

void UPPMainMenuWidget::HandleStartClicked()
{
	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>())
	{
		Controller->StartPoachingPatrolFromMenu();
	}
}

void UPPMainMenuWidget::HandleOptionsClicked()
{
	ShowOptionsMenu();
}

void UPPMainMenuWidget::HandleQuitClicked()
{
	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>())
	{
		Controller->QuitPoachingPatrolGame();
	}
}

void UPPMainMenuWidget::HandleOptionsBack()
{
	ShowRootMenu();
}
