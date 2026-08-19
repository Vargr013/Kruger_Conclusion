#include "UI/PPPauseMenuWidget.h"

#include "Kruger_ConclusionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/PPGraphicsSettingsWidget.h"
#include "Styling/CoreStyle.h"

namespace
{
	UButton* AddMenuButton(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, const FText& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 22));
		Button->AddChild(Text);
		Box->AddChildToVerticalBox(Button)->SetPadding(FMargin(8.0f));
		return Button;
	}
}

void UPPPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseCanvas"));
		WidgetTree->RootWidget = Canvas;
		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PauseBackdrop"));
		Backdrop->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.02f, 0.94f));
		Canvas->AddChildToCanvas(Backdrop)->SetAnchors(FAnchors(0.32f, 0.18f, 0.68f, 0.82f));

		UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseMenu"));
		Backdrop->AddChild(Menu);
		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
		Title->SetText(NSLOCTEXT("PoachingPatrol", "Paused", "Paused"));
		Title->SetJustification(ETextJustify::Center);
		Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 32));
		Menu->AddChildToVerticalBox(Title)->SetPadding(FMargin(12.0f));

		ResumeButton = AddMenuButton(WidgetTree, Menu, TEXT("Resume"), NSLOCTEXT("PoachingPatrol", "Resume", "Resume"));
		GraphicsSettings = WidgetTree->ConstructWidget<UPPGraphicsSettingsWidget>(UPPGraphicsSettingsWidget::StaticClass(), TEXT("PauseGraphicsSettings"));
		Menu->AddChildToVerticalBox(GraphicsSettings)->SetPadding(FMargin(12.0f));
		ReturnButton = AddMenuButton(WidgetTree, Menu, TEXT("ReturnToMainMenu"), NSLOCTEXT("PoachingPatrol", "ReturnToMenu", "Return to Main Menu"));
	}
	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPPPauseMenuWidget::Resume);
	ReturnButton->OnClicked.AddUniqueDynamic(this, &UPPPauseMenuWidget::ReturnToMainMenu);
}

void UPPPauseMenuWidget::Resume()
{
	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>()) Controller->ClosePauseOverlay();
}

void UPPPauseMenuWidget::ReturnToMainMenu()
{
	if (AKruger_ConclusionPlayerController* Controller = GetOwningPlayer<AKruger_ConclusionPlayerController>()) Controller->ReturnToPoachingPatrolMenu();
}
