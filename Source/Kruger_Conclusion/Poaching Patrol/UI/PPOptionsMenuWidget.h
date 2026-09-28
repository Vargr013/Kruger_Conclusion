#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PPOptionsMenuWidget.generated.h"

class UButton;
class UPPGraphicsSettingsWidget;
class USlider;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPPOptionsBackRequested);

UCLASS()
class KRUGER_CONCLUSION_API UPPOptionsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category="Poaching Patrol|Options")
	FOnPPOptionsBackRequested OnBackRequested;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleSensitivityChanged(float NewValue);
	void RefreshSensitivityLabel(float Value);

	UPROPERTY() TObjectPtr<UPPGraphicsSettingsWidget> GraphicsSettings;
	UPROPERTY() TObjectPtr<USlider> SensitivitySlider;
	UPROPERTY() TObjectPtr<UTextBlock> SensitivityValueText;
	UPROPERTY() TObjectPtr<UButton> BackButton;
};
