#include "UI/PPGraphicsSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
	UButton* AddPresetButton(UWidgetTree* Tree, UHorizontalBox* Row, const TCHAR* Name, TObjectPtr<UTextBlock>& OutText)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
		OutText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutText->SetText(FText::FromString(Name));
		OutText->SetJustification(ETextJustify::Center);
		Button->AddChild(OutText);
		if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Button))
		{
			Slot->SetPadding(FMargin(6.0f));
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		return Button;
	}
}

void UPPGraphicsSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsSettingsRoot"));
		WidgetTree->RootWidget = Root;
		HeadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GraphicsHeading"));
		HeadingText->SetText(NSLOCTEXT("PoachingPatrol", "GraphicsQuality", "Graphics Quality"));
		HeadingText->SetJustification(ETextJustify::Center);
		HeadingText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 22));
		Root->AddChildToVerticalBox(HeadingText)->SetPadding(FMargin(4.0f));

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PresetRow"));
		Root->AddChildToVerticalBox(Row)->SetPadding(FMargin(2.0f));
		LowButton = AddPresetButton(WidgetTree, Row, TEXT("Low"), LowText);
		MediumButton = AddPresetButton(WidgetTree, Row, TEXT("Medium"), MediumText);
		HighButton = AddPresetButton(WidgetTree, Row, TEXT("High"), HighText);
	}

	LowButton->OnClicked.AddUniqueDynamic(this, &UPPGraphicsSettingsWidget::SelectLow);
	MediumButton->OnClicked.AddUniqueDynamic(this, &UPPGraphicsSettingsWidget::SelectMedium);
	HighButton->OnClicked.AddUniqueDynamic(this, &UPPGraphicsSettingsWidget::SelectHigh);
	if (UPPGameUserSettings* Settings = UPPGameUserSettings::GetPPGameUserSettings())
	{
		Settings->OnGraphicsPresetChanged.AddUniqueDynamic(this, &UPPGraphicsSettingsWidget::HandlePresetChanged);
		RefreshSelection(Settings->GetGraphicsPreset());
	}
}

void UPPGraphicsSettingsWidget::SetHeading(const FText& Heading)
{
	if (HeadingText) HeadingText->SetText(Heading);
}

void UPPGraphicsSettingsWidget::RefreshSelection(EPPGraphicsPreset Preset)
{
	const FLinearColor Active(0.15f, 0.8f, 0.35f, 1.0f);
	const FLinearColor Inactive = FLinearColor::White;
	if (LowText) LowText->SetColorAndOpacity(Preset == EPPGraphicsPreset::Low ? Active : Inactive);
	if (MediumText) MediumText->SetColorAndOpacity(Preset == EPPGraphicsPreset::Medium ? Active : Inactive);
	if (HighText) HighText->SetColorAndOpacity(Preset == EPPGraphicsPreset::High ? Active : Inactive);
}

void UPPGraphicsSettingsWidget::Select(EPPGraphicsPreset Preset)
{
	if (UPPGameUserSettings* Settings = UPPGameUserSettings::GetPPGameUserSettings()) Settings->ApplyGraphicsPreset(Preset);
}

void UPPGraphicsSettingsWidget::SelectLow() { Select(EPPGraphicsPreset::Low); }
void UPPGraphicsSettingsWidget::SelectMedium() { Select(EPPGraphicsPreset::Medium); }
void UPPGraphicsSettingsWidget::SelectHigh() { Select(EPPGraphicsPreset::High); }
void UPPGraphicsSettingsWidget::HandlePresetChanged(EPPGraphicsPreset Preset) { RefreshSelection(Preset); }
