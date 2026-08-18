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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|State")
	EPPPoacherState CurrentPoacherState = EPPPoacherState::DisguisedRoaming;

	UPROPERTY(BlueprintReadWrite, Category="Poacher|Runtime")
	AActor* CurrentTargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture")
	float FollowDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture")
	float FollowAcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture")
	float CapturedMoveSpeed = 550.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Combat")
	float CombatMoveSpeed = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Combat")
	float CombatHoldDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Combat")
	float CombatLeashDistance = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Combat")
	float PlayerTargetMemoryTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Combat")
	float AttackWindupDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Combat", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ProjectileSubdualHealthFraction = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture")
	bool bIsCaptured = false;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Capture")
	AActor* CaptorActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Capture")
	bool bCaptureAttemptInProgress = false;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Combat")
	bool bIsSubdued = false;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Combat")
	bool bPlayerAttackWindupActive = false;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Combat")
	bool bHasPlayerAttackSlot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Capture", meta=(ClampMin="0.0"))
	float CaptureRetryLockDuration = 3.0f;

	float CaptureRetryLockedUntil = 0.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Escape", meta=(ClampMin="0.0"))
	float PredatorRemovalDelay = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Escape")
	bool bProcessedAsEscaped = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Status Effects")
	float PepperSprayDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Status Effects")
	float PepperSpraySpeedMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Poacher|Status Effects")
	float MinimumPepperSprayedSpeed = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poacher|Status Effects")
	bool bIsPepperSprayed = false;

	float IdleEndTime = 0.0f;
	float FleeEndTime = 0.0f;
	float LastEscapeUpdateTime = 0.0f;
	float NextIdleLocalWanderTime = 0.0f;
	float LastPlayerDetectedTime = -1000.0f;
	FTimerHandle ArrestRemovalTimerHandle;
	FTimerHandle PepperSprayTimerHandle;
	FTimerHandle PredatorRemovalTimerHandle;
	FTimerHandle PlayerAttackWindupTimerHandle;
	FTimerHandle SubdualTimerHandle;
	bool bSubduedByProjectile = false;

public:
	virtual void UpdateCreatureAI() override;
	virtual bool IsValidThreatActor_Implementation(AActor* PotentialThreat) const override;
	virtual void EnterFleeState(AActor* ThreatActor) override;
	virtual void EnterRoamState() override;

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

	UFUNCTION(BlueprintCallable, Category="Poacher|Combat")
	void StartEngagingPlayer(AActor* PlayerActor);

	UFUNCTION(BlueprintCallable, Category="Poacher|Combat")
	void CancelPlayerAttack();

	UFUNCTION(BlueprintCallable, Category="Poacher|Combat")
	void EnterSubduedState(float Duration = 4.0f, bool bFromProjectile = false);

	UFUNCTION(BlueprintPure, Category="Poacher|Combat")
	bool IsSubdued() const { return bIsSubdued; }

	UFUNCTION(BlueprintPure, Category="Poacher|Combat")
	bool IsEngagingPlayer() const { return CurrentPoacherState == EPPPoacherState::EngagingPlayer; }

	UFUNCTION(BlueprintPure, Category="Poacher|Combat")
	bool IsPlayerAttackWindupActive() const { return bPlayerAttackWindupActive; }

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	void CapturePoacher(AActor* NewCaptor);

	UFUNCTION(BlueprintPure, Category="Poacher|Capture")
	bool CanStartCaptureAttempt() const;

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	bool BeginCaptureAttempt();

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	bool ResolveCaptureAttempt(EPPRestraintResult Result, AActor* AttemptingCaptor);

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	void AbortCaptureAttempt();

	UFUNCTION(BlueprintPure, Category="Poacher|Capture")
	bool IsCaptureAttemptInProgress() const { return bCaptureAttemptInProgress; }

	UFUNCTION(BlueprintPure, Category="Poacher|Capture")
	bool IsCaptureRetryLocked() const;

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	void FollowCaptor();

	UFUNCTION(BlueprintCallable, Category="Poacher|Capture")
	bool MarkArrested();

	UFUNCTION(BlueprintCallable, Category="Poacher|Arrest")
	void RemoveFromLevelAfterArrest(float Delay);

	UFUNCTION(BlueprintCallable, Category="Poacher|Escape")
	void EscapePoacher();

	UFUNCTION(BlueprintPure, Category="Poacher|Escape")
	bool IsEscortTooFarFromCaptor() const;

	UFUNCTION(BlueprintCallable, Category="Poacher|Escape")
	void UpdateEscapePressure();

	UFUNCTION(BlueprintCallable, Category="Poacher|Status Effects")
	void ApplyPepperSpraySlow(float Duration = -1.0f);

	UFUNCTION(BlueprintPure, Category="Poacher|Status Effects")
	bool IsPepperSprayed() const { return bIsPepperSprayed; }

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

	UFUNCTION(BlueprintPure, Category="Poacher|Capture")
	AActor* GetCaptorActor() const { return CaptorActor; }

	UFUNCTION(BlueprintPure, Category="Poacher|Escape")
	float GetEscapeProgress() const { return EscapeProgress; }

	UFUNCTION(BlueprintPure, Category="Poacher|Escape")
	float GetEscapeGraceTime() const { return EscapeGraceTime; }

	UFUNCTION(BlueprintPure, Category="Poacher|Escape")
	float GetEscapeDistance() const { return EscapeDistance; }

	UFUNCTION(BlueprintPure, Category="Poacher|Escape")
	bool WasProcessedAsPermanentlyEscaped() const { return bProcessedAsEscaped; }

	UFUNCTION(BlueprintImplementableEvent, Category="Poacher|Combat", meta=(DisplayName="On Poacher Engaged Player"))
	void BP_OnEngagedPlayer(AActor* PlayerActor);

	UFUNCTION(BlueprintImplementableEvent, Category="Poacher|Combat", meta=(DisplayName="On Poacher Attack Windup"))
	void BP_OnAttackWindup(AActor* PlayerActor);

	UFUNCTION(BlueprintImplementableEvent, Category="Poacher|Combat", meta=(DisplayName="On Poacher Attack Hit"))
	void BP_OnAttackHit(AActor* PlayerActor);

	UFUNCTION(BlueprintImplementableEvent, Category="Poacher|Combat", meta=(DisplayName="On Poacher Attack Miss"))
	void BP_OnAttackMiss(AActor* PlayerActor);

	UFUNCTION(BlueprintImplementableEvent, Category="Poacher|Combat", meta=(DisplayName="On Poacher Disoriented"))
	void BP_OnDisoriented();

protected:
	void UpdateIdleLocalWander(float CurrentTime);
	void UpdatePlayerCombat(AActor* PlayerActor, float CurrentTime);
	bool CanDetectPlayerForCombat(AActor* PlayerActor) const;
	bool HasClearLineOfSightTo(AActor* TargetActor) const;
	bool IsWithinCombatLeash(AActor* PlayerActor) const;
	bool TryStartPlayerAttack(AActor* PlayerActor, float CurrentTime);
	void ResolvePlayerAttack();
	void DisengageFromPlayer(bool bReturnToIdle);
	void ClearSubduedState(bool bResumeBehaviour);
	void BroadcastCombatEvent(EPPPoacherCombatEvent Event) const;
	bool IsPlayerProjectileDamage(AController* EventInstigator, AActor* DamageCauser) const;
	virtual void HandleHealthDepleted() override;
	virtual bool CanAttackTarget(AActor* PotentialTarget) const override;
	float GetAdjustedPoacherMoveSpeed(float BaseSpeed) const;
	void RefreshPoacherMoveSpeed();

	UFUNCTION()
	void FinalizeArrestRemoval();

	UFUNCTION()
	void FinalizePredatorRemoval();

	UFUNCTION()
	void ClearPepperSpraySlow();

	UFUNCTION()
	void RecoverFromTimedSubdual();
};
