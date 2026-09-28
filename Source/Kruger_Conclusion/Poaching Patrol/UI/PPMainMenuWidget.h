#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PPMainMenuWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UPPOptionsMenuWidget;
class UScaleBox;
class UTexture2D;
class UVerticalBox;

UCLASS()
class KRUGER_CONCLUSION_API UPPMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPPMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	void ShowRootMenu();
	void ShowOptionsMenu();

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION() void HandleStartClicked();
	UFUNCTION() void HandleOptionsClicked();
	UFUNCTION() void HandleQuitClicked();
	UFUNCTION() void HandleOptionsBack();

	UPROPERTY() TObjectPtr<UTexture2D> MainMenuTexture;
	UPROPERTY() TObjectPtr<UImage> BackgroundImage;
	UPROPERTY() TObjectPtr<UScaleBox> ButtonScale;
	UPROPERTY() TObjectPtr<UBorder> ButtonShade;
	UPROPERTY() TObjectPtr<UScaleBox> OptionsScale;
	UPROPERTY() TObjectPtr<UBorder> OptionsPanel;
	UPROPERTY() TObjectPtr<UVerticalBox> RootMenu;
	UPROPERTY() TObjectPtr<UPPOptionsMenuWidget> OptionsMenu;
	UPROPERTY() TObjectPtr<UButton> StartButton;
	UPROPERTY() TObjectPtr<UButton> OptionsButton;
	UPROPERTY() TObjectPtr<UButton> QuitButton;
};
