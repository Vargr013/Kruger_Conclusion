#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PPGameUserSettings.h"
#include "PPGraphicsSettingsWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class KRUGER_CONCLUSION_API UPPGraphicsSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHeading(const FText& Heading);
	void RefreshSelection(EPPGraphicsPreset Preset);

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION() void SelectLow();
	UFUNCTION() void SelectMedium();
	UFUNCTION() void SelectHigh();
	UFUNCTION() void HandlePresetChanged(EPPGraphicsPreset Preset);
	void Select(EPPGraphicsPreset Preset);

	UPROPERTY() TObjectPtr<UTextBlock> HeadingText;
	UPROPERTY() TObjectPtr<UButton> LowButton;
	UPROPERTY() TObjectPtr<UButton> MediumButton;
	UPROPERTY() TObjectPtr<UButton> HighButton;
	UPROPERTY() TObjectPtr<UTextBlock> LowText;
	UPROPERTY() TObjectPtr<UTextBlock> MediumText;
	UPROPERTY() TObjectPtr<UTextBlock> HighText;
};
