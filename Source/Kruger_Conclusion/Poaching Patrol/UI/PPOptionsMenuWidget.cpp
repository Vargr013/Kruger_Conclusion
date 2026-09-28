#include "UI/PPOptionsMenuWidget.h"
#include "UI/PPUIStyle.h"
#include "UI/PPGraphicsSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/PPGameUserSettings.h"

void UPPOptionsMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionsRoot"));
		WidgetTree->RootWidget = Root;

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OptionsTitle"));
		Title->SetText(NSLOCTEXT("PoachingPatrol", "Options", "Options"));
		Title->SetJustification(ETextJustify::Center);
		Title->SetFont(PPUIStyle::Font(TEXT("Bold"), 30));
		Root->AddChildToVerticalBox(Title)->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 10.0f));

		GraphicsSettings = WidgetTree->ConstructWidget<UPPGraphicsSettingsWidget>(
			UPPGraphicsSettingsWidget::StaticClass(), TEXT("OptionsGraphicsSettings"));
		Root->AddChildToVerticalBox(GraphicsSettings)->SetPadding(FMargin(8.0f, 4.0f));

		UBorder* SensitivityFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SensitivityFrame"));
		SensitivityFrame->SetBrush(PPUIStyle::PanelBrush(FLinearColor(0.11f, 0.085f, 0.045f, 0.96f)));
		SensitivityFrame->SetPadding(FMargin(10.0f, 8.0f));
		Root->AddChildToVerticalBox(SensitivityFrame)->SetPadding(FMargin(8.0f, 10.0f));

		UVerticalBox* SensitivityColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SensitivityColumn"));
		SensitivityFrame->SetContent(SensitivityColumn);

		UTextBlock* SensitivityHeading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SensitivityHeading"));
		SensitivityHeading->SetText(NSLOCTEXT("PoachingPatrol", "MouseSensitivity", "Mouse Sensitivity"));
		SensitivityHeading->SetJustification(ETextJustify::Center);
		SensitivityHeading->SetFont(PPUIStyle::Font(TEXT("Bold"), 20));
		SensitivityColumn->AddChildToVerticalBox(SensitivityHeading)->SetPadding(FMargin(4.0f, 2.0f));

		SensitivityValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SensitivityValue"));
		SensitivityValueText->SetJustification(ETextJustify::Center);
		SensitivityValueText->SetFont(PPUIStyle::Font(TEXT("Regular"), 16));
		SensitivityColumn->AddChildToVerticalBox(SensitivityValueText)->SetPadding(FMargin(4.0f, 2.0f));

		SensitivitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("SensitivitySlider"));
		SensitivitySlider->SetMinValue(UPPGameUserSettings::MinMouseSensitivity);
		SensitivitySlider->SetMaxValue(UPPGameUserSettings::MaxMouseSensitivity);
		SensitivitySlider->SetStepSize(0.05f);
		SensitivitySlider->SetValue(UPPGameUserSettings::DefaultMouseSensitivity);
		SensitivityColumn->AddChildToVerticalBox(SensitivitySlider)->SetPadding(FMargin(8.0f, 8.0f, 8.0f, 4.0f));

		BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("OptionsBack"));
		PPUIStyle::OutlineButton(BackButton);
		UTextBlock* BackText = WidgetTree->ConstructWidget<UTextBlock>();
		BackText->SetText(NSLOCTEXT("PoachingPatrol", "OptionsBack", "Back"));
		BackText->SetJustification(ETextJustify::Center);
		BackText->SetFont(PPUIStyle::Font(TEXT("Bold"), 22));
		BackButton->AddChild(BackText);
		Root->AddChildToVerticalBox(BackButton)->SetPadding(FMargin(8.0f, 14.0f, 8.0f, 4.0f));
	}

	BackButton->OnClicked.AddUniqueDynamic(this, &UPPOptionsMenuWidget::HandleBackClicked);
	SensitivitySlider->OnValueChanged.AddUniqueDynamic(this, &UPPOptionsMenuWidget::HandleSensitivityChanged);

	float CurrentSensitivity = UPPGameUserSettings::DefaultMouseSensitivity;
	if (UPPGameUserSettings* Settings = UPPGameUserSettings::GetPPGameUserSettings())
	{
		CurrentSensitivity = Settings->GetMouseSensitivity();
	}
	SensitivitySlider->SetValue(CurrentSensitivity);
	RefreshSensitivityLabel(CurrentSensitivity);
}

void UPPOptionsMenuWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}

void UPPOptionsMenuWidget::HandleSensitivityChanged(float NewValue)
{
	if (UPPGameUserSettings* Settings = UPPGameUserSettings::GetPPGameUserSettings())
	{
		Settings->SetMouseSensitivity(NewValue);
		RefreshSensitivityLabel(Settings->GetMouseSensitivity());
		return;
	}
	RefreshSensitivityLabel(NewValue);
}

void UPPOptionsMenuWidget::RefreshSensitivityLabel(float Value)
{
	if (SensitivityValueText)
	{
		SensitivityValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2fx"), Value)));
	}
}
