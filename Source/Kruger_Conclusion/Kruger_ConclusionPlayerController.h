// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "GameFramework/PlayerController.h"
#include "Kruger_ConclusionPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UPPPatrolHUDWidget;
class UPPRoundReportWidget;
class UPPRestraintMinigameWidget;
class UPPPauseMenuWidget;
class UPPGraphicsSettingsWidget;
class APPPoacherCharacter;
struct FPPRoundResult;
enum class EPPRestraintResult : uint8;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class KRUGER_CONCLUSION_API AKruger_ConclusionPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AKruger_ConclusionPlayerController();

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Flow")
	void ReplayPoachingPatrolDay();

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Flow")
	void ReturnToPoachingPatrolMenu();

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Graphics")
	void OpenGraphicsSettings();

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Graphics")
	void CloseGraphicsSettings();

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Pause")
	void OpenPauseOverlay();

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Pause")
	void ClosePauseOverlay();

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Restraint")
	bool StartPoacherRestraint(APPPoacherCharacter* Poacher);

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditAnywhere, Category="HUD|Poaching Patrol")
	bool bShowPoachingPatrolHUD = true;

	UPROPERTY(EditAnywhere, Category="HUD|Poaching Patrol")
	TSubclassOf<UPPPatrolHUDWidget> PoachingPatrolHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="HUD|Poaching Patrol|Input")
	TObjectPtr<UInputAction> MinimapZoomAction;

	UPROPERTY()
	TObjectPtr<UPPPatrolHUDWidget> PoachingPatrolHUDWidget;

	UPROPERTY(EditAnywhere, Category="HUD|Poaching Patrol")
	TSubclassOf<UPPRoundReportWidget> RoundReportWidgetClass;

	UPROPERTY()
	TObjectPtr<UPPRoundReportWidget> RoundReportWidget;

	UPROPERTY(EditAnywhere, Category="HUD|Poaching Patrol")
	TSubclassOf<UPPRestraintMinigameWidget> RestraintMinigameWidgetClass;

	UPROPERTY()
	TObjectPtr<UPPRestraintMinigameWidget> RestraintMinigameWidget;

	UPROPERTY()
	TObjectPtr<UPPPauseMenuWidget> PauseMenuWidget;

	UPROPERTY()
	TObjectPtr<UPPGraphicsSettingsWidget> MainMenuGraphicsWidget;

	UPROPERTY()
	TWeakObjectPtr<APPPoacherCharacter> ActiveRestraintPoacher;

	TWeakObjectPtr<APawn> ActiveRestraintCaptor;
	ESlateVisibility PreviousPatrolHUDVisibility = ESlateVisibility::Visible;

	UFUNCTION()
	void HandlePoachingPatrolRoundEnded(FPPRoundResult Result);

	UFUNCTION()
	void HandleRestraintFinished(EPPRestraintResult Result);

	void HandleMinimapZoom();
	void ToggleGameplayPause();
	bool IsLegacyMainMenuVisible() const;
	void RefreshMainMenuGraphicsEntry();
	void RestoreGameplayAfterRestraint();
	void AbortActiveRestraint();

	void ApplyReplayMenuBypass();
	void ReloadCurrentPatrolLevel();
	FTimerHandle ReplayMenuBypassTimer;
	FTimerHandle MainMenuGraphicsTimer;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
};
