#pragma once

#include "CoreMinimal.h"
#include "Characters/PPCreatureBase.h"
#include "Data/PPGameTypes.h"
#include "Interfaces/PPInteractableInterface.h"
#include "PPPoacherCharacter.generated.h"

class AActor;

UCLASS()
class KRUGER_CONCLUSION_API APPPoacherCharacter : public APPCreatureBase, public IPPInteractableInterface
{
	GENERATED_BODY()

public:
	APPPoacherCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|State")
	EPPPoacherState CurrentPoacherState = EPPPoacherState::DisguisedRoaming;

	UPROPERTY(BlueprintReadWrite, Category="Poacher|Runtime")
	AActor* CurrentTargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture")
	float FollowDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture")
	float FollowAcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture")
	float CapturedMoveSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture")
	bool bIsCaptured = false;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Capture")
	AActor* CaptorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Timing")
	float IdleTimeMin = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Timing")
	float IdleTimeMax = 6.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Escape")
	float EscapeDistance = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Escape")
	float EscapeGraceTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Escape")
	bool bCanEscapeAfterCapture = true;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Escape")
	float EscapeProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Arrest")
	bool bPendingRemovalAfterArrest = false;

	float IdleEndTime = 0.0f;
	float FleeEndTime = 0.0f;
	float LastEscapeUpdateTime = 0.0f;
	float NextIdleLocalWanderTime = 0.0f;
	FTimerHandle ArrestRemovalTimerHandle;

public:
	virtual void UpdateCreatureAI() override;
	virtual bool IsValidThreatActor_Implementation(AActor* PotentialThreat) const override;

	UFUNCTION(BlueprintCallable, Category="Poacher|State")
	void SetPoacherState(EPPPoacherState NewState);

	UFUNCTION(BlueprintPure, Category="Poacher|State")
	EPPPoacherState GetPoacherState() const { return CurrentPoacherState; }

	UFUNCTION(BlueprintCallable, Category="Poacher|Behaviour")
	void StartDisguisedRoaming();

	UFUNCTION(BlueprintCallable, Category="Poacher|Behaviour")
	void StartDisguisedIdle();

	UFUNCTION(BlueprintCallable, Category="Poacher|Behaviour")
	void StartFleeing(AActor* ThreatActor);

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	void CapturePoacher(AActor* NewCaptor);

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	void FollowCaptor();

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	void MarkArrested();

	UFUNCTION(BlueprintCallable, Category="Poacher|Arrest")
	void RemoveFromLevelAfterArrest(float Delay);

	UFUNCTION(BlueprintCallable, Category="Poacher|Escape")
	void EscapePoacher();

	UFUNCTION(BlueprintPure, Category="Poacher|Escape")
	bool IsEscortTooFarFromCaptor() const;

	UFUNCTION(BlueprintCallable, Category="Poacher|Escape")
	void UpdateEscapePressure();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category="Poacher|AI")
	void SetTargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category="Poacher|AI")
	AActor* GetTargetActor() const { return CurrentTargetActor; }

	UFUNCTION(BlueprintPure, Category="Poacher|AI")
	bool HasTarget() const { return CurrentTargetActor != nullptr; }

	UFUNCTION(BlueprintCallable, Category="Poacher|AI")
	void SetThreatActor(AActor* NewThreat);

	UFUNCTION(BlueprintPure, Category="Poacher|Capture")
	bool IsCaptured() const { return bIsCaptured; }

	UFUNCTION(BlueprintPure, Category="Poacher|Escape")
	float GetEscapeProgress() const { return EscapeProgress; }

protected:
	void UpdateIdleLocalWander(float CurrentTime);

	UFUNCTION()
	void FinalizeArrestRemoval();
};
