#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PPPauseMenuWidget.generated.h"

class UButton;
class UPPOptionsMenuWidget;
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
	UFUNCTION() void OpenOptions();
	UFUNCTION() void CloseOptions();
	UFUNCTION() void ReturnToMainMenu();
	UFUNCTION() void RequestModeSwitch();
	UFUNCTION() void ConfirmModeSwitch();
	UFUNCTION() void CancelModeSwitch();
	void RefreshPatrolMode();
	void ShowMainMenu();
	void ShowOptionsMenu();

	UPROPERTY() TObjectPtr<UVerticalBox> MainMenu;
	UPROPERTY() TObjectPtr<UPPOptionsMenuWidget> OptionsMenu;
	UPROPERTY() TObjectPtr<UButton> ResumeButton;
	UPROPERTY() TObjectPtr<UButton> OptionsButton;
	UPROPERTY() TObjectPtr<UButton> ReturnButton;
	UPROPERTY() TObjectPtr<UButton> ModeSwitchButton;
	UPROPERTY() TObjectPtr<UButton> ConfirmModeButton;
	UPROPERTY() TObjectPtr<UButton> CancelModeButton;
	UPROPERTY() TObjectPtr<UTextBlock> CurrentModeText;
	UPROPERTY() TObjectPtr<UTextBlock> ModeSwitchButtonText;
	UPROPERTY() TObjectPtr<UTextBlock> ConfirmModeButtonText;
	UPROPERTY() TObjectPtr<UVerticalBox> ModeConfirmation;
	bool bRequestedTutorialMode = false;
	bool bModeRestartRequested = false;
};
