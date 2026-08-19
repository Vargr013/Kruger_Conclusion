#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PPPauseMenuWidget.generated.h"

class UButton;
class UPPGraphicsSettingsWidget;

UCLASS()
class KRUGER_CONCLUSION_API UPPPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION() void Resume();
	UFUNCTION() void ReturnToMainMenu();

	UPROPERTY() TObjectPtr<UButton> ResumeButton;
	UPROPERTY() TObjectPtr<UButton> ReturnButton;
	UPROPERTY() TObjectPtr<UPPGraphicsSettingsWidget> GraphicsSettings;
};
