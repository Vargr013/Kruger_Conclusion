#include "UI/PPRoundReportWidget.h"

#include "Kruger_ConclusionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	UTextBlock* AddReportText(UWidgetTree* Tree, UVerticalBox* Parent, const TCHAR* Name, int32 Size, const FLinearColor& Color, ETextJustify::Type Justification = ETextJustify::Center)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justification);
		Text->SetAutoWrapText(true);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Text))
		{
			Slot->SetPadding(FMargin(18.0f, 7.0f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
		return Text;
	}
}

void UPPRoundReportWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget)
	{
		BuildDefaultWidgetTree();
	}
	RefreshText();
}

void UPPRoundReportWidget::BuildDefaultWidgetTree()
{
	const FLinearColor Ink(0.11f, 0.085f, 0.045f, 1.0f);
	const FLinearColor Paper(0.84f, 0.76f, 0.58f, 0.98f);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ReportCanvas"));
	WidgetTree->RootWidget = Canvas;

	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dimmer"));
	Dimmer->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.008f, 0.84f));
	if (UCanvasPanelSlot* DimmerSlot = Canvas->AddChildToCanvas(Dimmer))
	{
		DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		DimmerSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* PaperBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Newspaper"));
	PaperBorder->SetBrushColor(Paper);
	PaperBorder->SetPadding(FMargin(30.0f));
	if (UCanvasPanelSlot* PaperSlot = Canvas->AddChildToCanvas(PaperBorder))
	{
		PaperSlot->SetAnchors(FAnchors(0.5f));
		PaperSlot->SetAlignment(FVector2D(0.5f));
		PaperSlot->SetSize(FVector2D(760.0f, 620.0f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ReportColumn"));
	PaperBorder->SetContent(Column);

	MastheadText = AddReportText(WidgetTree, Column, TEXT("Masthead"), 38, Ink);
	HeadlineText = AddReportText(WidgetTree, Column, TEXT("Headline"), 30, Ink);
	OutcomeText = AddReportText(WidgetTree, Column, TEXT("Outcome"), 18, Ink);
	PoacherText = AddReportText(WidgetTree, Column, TEXT("Poachers"), 22, Ink, ETextJustify::Left);
	AnimalText = AddReportText(WidgetTree, Column, TEXT("Animals"), 22, Ink, ETextJustify::Left);
	FooterText = AddReportText(WidgetTree, Column, TEXT("Footer"), 14, Ink);

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ButtonSpacer"));
	Spacer->SetSize(FVector2D(1.0f, 12.0f));
	Column->AddChildToVerticalBox(Spacer);

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = Column->AddChildToVerticalBox(ButtonRow))
	{
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonRowSlot->SetPadding(FMargin(0.0f, 12.0f));
	}

	auto AddButton = [this, ButtonRow, Ink, Paper](const TCHAR* ButtonName, const TCHAR* Label, void (UPPRoundReportWidget::*Handler)())
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		Button->SetBackgroundColor(Ink);
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>();
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetColorAndOpacity(FSlateColor(Paper));
		FSlateFontInfo Font = LabelText->GetFont();
		Font.Size = 17;
		LabelText->SetFont(Font);
		Button->SetContent(LabelText);
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(LabelText->Slot))
		{
			ContentSlot->SetPadding(FMargin(22.0f, 10.0f));
		}
		if (UHorizontalBoxSlot* Slot = ButtonRow->AddChildToHorizontalBox(Button))
		{
			Slot->SetPadding(FMargin(10.0f, 0.0f));
		}
		if (Handler == &UPPRoundReportWidget::HandleReplayClicked)
		{
			Button->OnClicked.AddDynamic(this, &UPPRoundReportWidget::HandleReplayClicked);
		}
		else
		{
			Button->OnClicked.AddDynamic(this, &UPPRoundReportWidget::HandleMainMenuClicked);
		}
	};

	AddButton(TEXT("ReplayButton"), TEXT("REPLAY DAY"), &UPPRoundReportWidget::HandleReplayClicked);
	AddButton(TEXT("MainMenuButton"), TEXT("MAIN MENU"), &UPPRoundReportWidget::HandleMainMenuClicked);
}

void UPPRoundReportWidget::SetRoundResult(const FPPRoundResult& InResult)
{
	RoundResult = InResult;
	RefreshText();
}

void UPPRoundReportWidget::RefreshText()
{
	if (!MastheadText)
	{
		return;
	}

	const FPPRoundSnapshot& Snapshot = RoundResult.Snapshot;
	const bool bSuccess = RoundResult.Outcome == EPPRoundOutcome::Success;
	MastheadText->SetText(FText::FromString(TEXT("THE KRUGER DAILY")));
	HeadlineText->SetText(FText::FromString(bSuccess ? TEXT("RANGER PATROL SECURES THE RESERVE") : TEXT("POACHING QUOTA MISSED")));
	OutcomeText->SetText(FText::FromString(bSuccess ? TEXT("DAY RESULT: PATROL SUCCESS") : TEXT("DAY RESULT: PATROL FAILED")));
	PoacherText->SetText(FText::FromString(FString::Printf(
		TEXT("POACHERS ARRESTED\n%d of %d caught  |  %d required  |  %.0f%% capture rate"),
		Snapshot.PoachersArrested,
		Snapshot.TotalPoachers,
		Snapshot.RequiredArrests,
		Snapshot.CaptureRate * 100.0f)));
	AnimalText->SetText(FText::FromString(FString::Printf(
		TEXT("WILDLIFE REPORT\n%d of %d animals saved  |  %d poached"),
		Snapshot.AnimalsAlive,
		Snapshot.TotalAnimals,
		Snapshot.AnimalsPoached)));
	FooterText->SetText(FText::FromString(TEXT("Skukuza Reserve • End-of-day field report")));
}

void UPPRoundReportWidget::HandleReplayClicked()
{
	if (AKruger_ConclusionPlayerController* Controller = Cast<AKruger_ConclusionPlayerController>(GetOwningPlayer()))
	{
		Controller->ReplayPoachingPatrolDay();
	}
}

void UPPRoundReportWidget::HandleMainMenuClicked()
{
	if (AKruger_ConclusionPlayerController* Controller = Cast<AKruger_ConclusionPlayerController>(GetOwningPlayer()))
	{
		Controller->ReturnToPoachingPatrolMenu();
	}
}
