#include "UI/PPPauseMenuWidget.h"
#include "UI/PPUIStyle.h"

#include "Kruger_ConclusionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EnvironmentLevelSubsystem.h"
#include "UI/PPGraphicsSettingsWidget.h"
#include "Styling/CoreStyle.h"

namespace
{
	UButton* AddMenuButton(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, const FText& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
		PPUIStyle::OutlineButton(Button);
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Text->SetFont(PPUIStyle::Font(TEXT("Bold"), 22));
		Button->AddChild(Text);
		Box->AddChildToVerticalBox(Button)->SetPadding(FMargin(8.0f));
		return Button;
	}
}

void UPPPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseCanvas"));
		WidgetTree->RootWidget = Canvas;
		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PauseBackdrop"));
		Backdrop->SetBrush(PPUIStyle::PanelBrush(FLinearColor(0.11f, 0.085f, 0.045f, 0.96f)));
		Canvas->AddChildToCanvas(Backdrop)->SetAnchors(FAnchors(0.32f, 0.18f, 0.68f, 0.82f));

		UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseMenu"));
		Backdrop->AddChild(Menu);
		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
		Title->SetText(NSLOCTEXT("PoachingPatrol", "Paused", "Paused"));
		Title->SetJustification(ETextJustify::Center);
		Title->SetFont(PPUIStyle::Font(TEXT("Bold"), 32));
		Menu->AddChildToVerticalBox(Title)->SetPadding(FMargin(12.0f));

		ResumeButton = AddMenuButton(WidgetTree, Menu, TEXT("Resume"), NSLOCTEXT("PoachingPatrol", "Resume", "Resume"));
		GraphicsSettings = WidgetTree->ConstructWidget<UPPGraphicsSettingsWidget>(UPPGraphicsSettingsWidget::StaticClass(), TEXT("PauseGraphicsSettings"));
		Menu->AddChildToVerticalBox(GraphicsSettings)->SetPadding(FMargin(12.0f));

		CurrentModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CurrentPatrolMode"));
		CurrentModeText->SetJustification(ETextJustify::Center);
		CurrentModeText->SetFont(PPUIStyle::Font(TEXT("Regular"), 19));
		CurrentModeText->SetAutoWrapText(true);
		Menu->AddChildToVerticalBox(CurrentModeText)->SetPadding(FMargin(12.0f, 14.0f, 12.0f, 4.0f));

		ModeSwitchButton = AddMenuButton(WidgetTree, Menu, TEXT("SwitchPatrolMode"), FText::GetEmpty());
		ModeSwitchButtonText = Cast<UTextBlock>(ModeSwitchButton->GetContent());

		ModeConfirmation = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModeConfirmation"));
		Menu->AddChildToVerticalBox(ModeConfirmation)->SetPadding(FMargin(8.0f));
		UTextBlock* Warning = WidgetTree->ConstructWidget<UTextBlock>();
		Warning->SetText(NSLOCTEXT("PoachingPatrol", "ModeRestartWarning", "Changing patrol mode restarts the level and resets your current progress."));
		Warning->SetJustification(ETextJustify::Center);
		Warning->SetFont(PPUIStyle::Font(TEXT("Regular"), 17));
		Warning->SetAutoWrapText(true);
		ModeConfirmation->AddChildToVerticalBox(Warning)->SetPadding(FMargin(10.0f));
		ConfirmModeButton = AddMenuButton(WidgetTree, ModeConfirmation, TEXT("ConfirmPatrolMode"), FText::GetEmpty());
		ConfirmModeButtonText = Cast<UTextBlock>(ConfirmModeButton->GetContent());
		CancelModeButton = AddMenuButton(WidgetTree, ModeConfirmation, TEXT("CancelPatrolMode"), NSLOCTEXT("PoachingPatrol", "CancelModeRestart", "Cancel"));
		ModeConfirmation->SetVisibility(ESlateVisibility::Collapsed);

		ReturnButton = AddMenuButton(WidgetTree, Menu, TEXT("ReturnToMainMenu"), NSLOCTEXT("PoachingPatrol", "ReturnToMenu", "Return to Main Menu"));
	}
	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPPPauseMenuWidget::Resume);
	ReturnButton->OnClicked.AddUniqueDynamic(this, &UPPPauseMenuWidget::ReturnToMainMenu);
	ModeSwitchButton->OnClicked.AddUniqueDynamic(this, &UPPPauseMenuWidget::RequestModeSwitch);
	ConfirmModeButton->OnClicked.AddUniqueDynamic(this, &UPPPauseMenuWidget::ConfirmModeSwitch);
	CancelModeButton->OnClicked.AddUniqueDynamic(this, &UPPPauseMenuWidget::CancelModeSwitch);
	RefreshPatrolMode();
}

void UPPPauseMenuWidget::Resume()
{
	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>()) Controller->ClosePauseOverlay();
}

void UPPPauseMenuWidget::ReturnToMainMenu()
{
	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>()) Controller->ReturnToPoachingPatrolMenu();
}

void UPPPauseMenuWidget::RefreshPatrolMode()
{
	UEnvironmentLevelSubsystem* Rules = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr;
	if (!Rules || !Rules->GetTutorialDirector())
	{
		CurrentModeText->SetText(NSLOCTEXT("PoachingPatrol", "PatrolModeUnavailable", "Patrol mode switching is unavailable in this level."));
		ModeSwitchButtonText->SetText(NSLOCTEXT("PoachingPatrol", "NoPatrolMode", "Mode unavailable"));
		ModeSwitchButton->SetIsEnabled(false);
		return;
	}

	const bool bTutorialMode = Rules->IsTutorialMode();
	CurrentModeText->SetText(bTutorialMode
		? NSLOCTEXT("PoachingPatrol", "CurrentTutorialMode", "Current patrol: First Day Tutorial")
		: NSLOCTEXT("PoachingPatrol", "CurrentNormalMode", "Current patrol: Normal Patrol"));
	bRequestedTutorialMode = !bTutorialMode;
	ModeSwitchButtonText->SetText(bRequestedTutorialMode
		? NSLOCTEXT("PoachingPatrol", "SwitchToTutorial", "Restart in Tutorial")
		: NSLOCTEXT("PoachingPatrol", "SwitchToNormal", "Restart in Normal Patrol"));
	ConfirmModeButtonText->SetText(bRequestedTutorialMode
		? NSLOCTEXT("PoachingPatrol", "ConfirmTutorialRestart", "Restart Tutorial")
		: NSLOCTEXT("PoachingPatrol", "ConfirmNormalRestart", "Restart Normal Patrol"));
	ModeSwitchButton->SetIsEnabled(true);
}

void UPPPauseMenuWidget::RequestModeSwitch()
{
	bModeRestartRequested = false;
	ModeSwitchButton->SetVisibility(ESlateVisibility::Collapsed);
	ModeConfirmation->SetVisibility(ESlateVisibility::Visible);
	ConfirmModeButton->SetKeyboardFocus();
}

void UPPPauseMenuWidget::ConfirmModeSwitch()
{
	if (bModeRestartRequested)
	{
		return;
	}
	bModeRestartRequested = true;
	ConfirmModeButton->SetIsEnabled(false);
	CancelModeButton->SetIsEnabled(false);

	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>())
	{
		if (Controller->RestartPoachingPatrolInMode(bRequestedTutorialMode))
		{
			return;
		}
	}

	CancelModeSwitch();
	RefreshPatrolMode();
}

void UPPPauseMenuWidget::CancelModeSwitch()
{
	bModeRestartRequested = false;
	ConfirmModeButton->SetIsEnabled(true);
	CancelModeButton->SetIsEnabled(true);
	ModeConfirmation->SetVisibility(ESlateVisibility::Collapsed);
	ModeSwitchButton->SetVisibility(ESlateVisibility::Visible);
	ModeSwitchButton->SetKeyboardFocus();
}
