#include "UI/PPTutorialWidget.h"
#include "UI/PPUIStyle.h"
#include "Actors/PPTutorialDirector.h"
#include "EnvironmentLevelSubsystem.h"
#include "Kruger_ConclusionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UPPTutorialWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	auto Panel = [&](FAnchors Anchors, FMargin Offsets, UBorder*& OutPanel)
	{
		OutPanel = WidgetTree->ConstructWidget<UBorder>();
		OutPanel->SetBrush(PPUIStyle::PanelBrush(FLinearColor(0.075f, 0.06f, 0.035f, 0.96f)));
		OutPanel->SetPadding(FMargin(14));
		auto* Slot = Canvas->AddChildToCanvas(OutPanel); Slot->SetAnchors(Anchors); Slot->SetOffsets(Offsets);
		auto* Box = WidgetTree->ConstructWidget<UVerticalBox>(); OutPanel->AddChild(Box); return Box;
	};
	auto Text = [&](UVerticalBox* Box, int32 Size, const FLinearColor& Color)
	{
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetFont(PPUIStyle::Font(TEXT("Regular"), Size)); Label->SetColorAndOpacity(Color);
		Label->SetAutoWrapText(true); Box->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 3)); return Label;
	};
	const FLinearColor Gold(0.96f, 0.72f, 0.35f), Cream(1, 0.95f, 0.84f);
	UBorder* Border = nullptr;
	auto* Box = Panel(FAnchors(0, 0), FMargin(24, 76, 368, 225), Border); ObjectivePanel = Border;
	Text(Box, 17, Gold)->SetText(FText::FromString(TEXT("FIRST DAY ON PATROL")));
	Objective = Text(Box, 17, Cream); Destination = Text(Box, 14, Gold); Hint = Text(Box, 13, Cream); Feedback = Text(Box, 13, FLinearColor(0.6f, 1, 0.5f));
	Box = Panel(FAnchors(0.23f, 0.69f, 0.77f, 0.86f), FMargin(0), Border); SubtitlePanel = Border;
	Speaker = Text(Box, 16, Gold); Subtitle = Text(Box, 19, Cream);
	Box = Panel(FAnchors(0.2f, 0.25f, 0.8f, 0.75f), FMargin(0), Border); ResultPanel = Border;
	ResultTitle = Text(Box, 30, Gold); ResultText = Text(Box, 21, Cream);
	auto* ReplayButton = WidgetTree->ConstructWidget<UButton>(); PPUIStyle::OutlineButton(ReplayButton);
	ReplayText = WidgetTree->ConstructWidget<UTextBlock>(); ReplayText->SetFont(PPUIStyle::Font(TEXT("Bold"), 23)); ReplayButton->AddChild(ReplayText);
	Box->AddChildToVerticalBox(ReplayButton)->SetPadding(FMargin(0, 20));
	ReplayButton->OnClicked.AddUniqueDynamic(this, &UPPTutorialWidget::Replay);
	auto* MenuButton = WidgetTree->ConstructWidget<UButton>(); PPUIStyle::OutlineButton(MenuButton);
	auto* MenuText = WidgetTree->ConstructWidget<UTextBlock>(); MenuText->SetText(FText::FromString(TEXT("Main Menu"))); MenuText->SetFont(PPUIStyle::Font(TEXT("Bold"), 23)); MenuButton->AddChild(MenuText);
	Box->AddChildToVerticalBox(MenuButton)->SetPadding(FMargin(0, 10));
	MenuButton->OnClicked.AddUniqueDynamic(this, &UPPTutorialWidget::MainMenu);
	ObjectivePanel->SetVisibility(ESlateVisibility::Collapsed); SubtitlePanel->SetVisibility(ESlateVisibility::Collapsed); ResultPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UPPTutorialWidget::ShowResult(bool bSuccess, const FText& Message)
{
	bShowingResult = true;
	ObjectivePanel->SetVisibility(ESlateVisibility::Collapsed); SubtitlePanel->SetVisibility(ESlateVisibility::Collapsed);
	ResultPanel->SetVisibility(ESlateVisibility::Visible);
	ResultTitle->SetText(FText::FromString(bSuccess ? TEXT("First Day on Patrol complete") : TEXT("Retry your first patrol")));
	ResultText->SetText(Message);
	ReplayText->SetText(FText::FromString(bSuccess ? TEXT("Replay Tutorial") : TEXT("Retry Tutorial")));
}

void UPPTutorialWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (bShowingResult) return;
	const auto* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>();
	const auto* Director = Rules && Rules->IsTutorialMode() ? Rules->GetTutorialDirector() : nullptr;
	auto* PC = GetOwningPlayer();
	const bool bShow = Director && Director->IsActive() && PC && !PC->IsPaused();
	ObjectivePanel->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SubtitlePanel->SetVisibility(bShow && !Director->GetSubtitle().IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bShow) return;
	Objective->SetText(Director->GetObjective()); Hint->SetText(Director->GetHint());
	Speaker->SetText(Director->GetSpeaker()); Subtitle->SetText(Director->GetSubtitle());
	Feedback->SetText(Director->HasRecentCompletion() ? FText::FromString(TEXT("Step complete")) : FText::GetEmpty());
	AActor* Target = Director->GetDestination();
	APawn* Pawn = PC->GetPawn();
	if (Target && Pawn)
	{
		const FVector Delta = Target->GetActorLocation() - Pawn->GetActorLocation();
		const float Angle = FMath::FindDeltaAngleDegrees(PC->GetControlRotation().Yaw, Delta.Rotation().Yaw);
		const TCHAR* Direction = FMath::Abs(Angle) < 25 ? TEXT("Ahead") : FMath::Abs(Angle) > 135 ? TEXT("Behind you") : Angle > 0 ? TEXT("To your right") : TEXT("To your left");
		Destination->SetText(FText::FromString(FString::Printf(TEXT("%s - %d m"), Direction, FMath::RoundToInt(Delta.Size2D() / 100))));
	}
	else Destination->SetText(FText::GetEmpty());
}

void UPPTutorialWidget::Replay()
{
	if (auto* PC = GetOwningPlayer<AKruger_ConclusionPlayerController>()) PC->ReplayPoachingPatrolDay();
}
void UPPTutorialWidget::MainMenu()
{
	if (auto* PC = GetOwningPlayer<AKruger_ConclusionPlayerController>()) PC->ReturnToPoachingPatrolMenu();
}
