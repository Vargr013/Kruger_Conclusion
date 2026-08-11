// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Kruger_ConclusionPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UPPPatrolHUDWidget;
class UPPRoundReportWidget;
struct FPPRoundResult;

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

	UPROPERTY()
	TObjectPtr<UPPPatrolHUDWidget> PoachingPatrolHUDWidget;

	UPROPERTY(EditAnywhere, Category="HUD|Poaching Patrol")
	TSubclassOf<UPPRoundReportWidget> RoundReportWidgetClass;

	UPROPERTY()
	TObjectPtr<UPPRoundReportWidget> RoundReportWidget;

	UFUNCTION()
	void HandlePoachingPatrolRoundEnded(FPPRoundResult Result);

	void ApplyReplayMenuBypass();
	void ReloadCurrentPatrolLevel();
	FTimerHandle ReplayMenuBypassTimer;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
};
