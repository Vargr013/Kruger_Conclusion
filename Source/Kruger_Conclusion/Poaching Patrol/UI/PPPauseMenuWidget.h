#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PPPauseMenuWidget.generated.h"

class UButton;
class UPPGraphicsSettingsWidget;
class UTextBlock;
class UVerticalBox;

UCLASS()
class KRUGER_CONCLUSION_API UPPPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION() void Resume();
	UFUNCTION() void ReturnToMainMenu();
	UFUNCTION() void RequestModeSwitch();
	UFUNCTION() void ConfirmModeSwitch();
	UFUNCTION() void CancelModeSwitch();
	void RefreshPatrolMode();

	UPROPERTY() TObjectPtr<UButton> ResumeButton;
	UPROPERTY() TObjectPtr<UButton> ReturnButton;
	UPROPERTY() TObjectPtr<UButton> ModeSwitchButton;
	UPROPERTY() TObjectPtr<UButton> ConfirmModeButton;
	UPROPERTY() TObjectPtr<UButton> CancelModeButton;
	UPROPERTY() TObjectPtr<UTextBlock> CurrentModeText;
	UPROPERTY() TObjectPtr<UTextBlock> ModeSwitchButtonText;
	UPROPERTY() TObjectPtr<UTextBlock> ConfirmModeButtonText;
	UPROPERTY() TObjectPtr<UVerticalBox> ModeConfirmation;
	UPROPERTY() TObjectPtr<UPPGraphicsSettingsWidget> GraphicsSettings;
	bool bRequestedTutorialMode = false;
	bool bModeRestartRequested = false;
};
