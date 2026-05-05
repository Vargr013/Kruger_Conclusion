#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "PPCreatureBase.generated.h"

class AAIController;

UCLASS(Abstract)
class KRUGER_CONCLUSION_API APPCreatureBase : public ACharacter
{
	GENERATED_BODY()

public:
	APPCreatureBase();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Movement")
	float WalkSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Movement")
	float FleeSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Roaming")
	float RoamRadius = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Roaming")
	float RoamAcceptanceRadius = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float ThreatDetectionRadius = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float ForwardThreatAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float FleeDistance = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Timing")
	float AIUpdateInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Timing")
	float MoveRequestCooldown = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	bool bDebugOnScreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	float DebugStatusInterval = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Creature|Runtime")
	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Creature|Runtime")
	FVector CurrentMoveTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Creature|Runtime")
	AActor* CurrentThreatActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Creature|Runtime")
	bool bHasActiveMoveTarget = false;

	UPROPERTY(BlueprintReadOnly, Category="Creature|Runtime")
	float LastMoveRequestTime = -1000.0f;

	float LastDebugStatusTime = -1000.0f;

	FTimerHandle AIUpdateTimerHandle;

public:
	UFUNCTION(BlueprintCallable, Category="Creature|AI")
	virtual void StartAIUpdates();

	UFUNCTION(BlueprintCallable, Category="Creature|AI")
	virtual void StopAIUpdates();

	UFUNCTION(BlueprintCallable, Category="Creature|AI")
	virtual void UpdateCreatureAI();

	UFUNCTION(BlueprintPure, Category="Creature|Threat")
	AActor* FindPlayerActor() const;

	UFUNCTION(BlueprintPure, Category="Creature|Threat")
	bool IsThreatInRange(AActor* ThreatActor) const;

	UFUNCTION(BlueprintPure, Category="Creature|Threat")
	bool IsThreatInForwardCone(AActor* ThreatActor) const;

	UFUNCTION(BlueprintPure, Category="Creature|Threat")
	bool ShouldFleeFromThreat(AActor* ThreatActor) const;

	UFUNCTION(BlueprintPure, Category="Creature|Movement")
	FVector GetDirectionAwayFromActor(AActor* ThreatActor) const;

	UFUNCTION(BlueprintPure, Category="Creature|Movement")
	FVector GetFleeDestination(AActor* ThreatActor) const;

	UFUNCTION(BlueprintCallable, Category="Creature|Movement")
	bool GetRandomRoamLocation(FVector& OutLocation) const;

	UFUNCTION(BlueprintCallable, Category="Creature|Movement")
	bool MoveToLocation(const FVector& TargetLocation, float AcceptanceRadius);

	UFUNCTION(BlueprintCallable, Category="Creature|Movement")
	void StopMovement();

	UFUNCTION(BlueprintCallable, Category="Creature|Movement")
	void SetCreatureMoveSpeed(float NewSpeed);

	UFUNCTION(BlueprintCallable, Category="Creature|AI")
	virtual void EnterFleeState(AActor* ThreatActor);

	UFUNCTION(BlueprintCallable, Category="Creature|AI")
	virtual void EnterRoamState();

	UFUNCTION(BlueprintPure, Category="Creature|Runtime")
	AActor* GetThreatActor() const { return CurrentThreatActor; }

	UFUNCTION(BlueprintPure, Category="Creature|Runtime")
	bool HasThreat() const { return CurrentThreatActor != nullptr; }

	UFUNCTION(BlueprintPure, Category="Creature|Runtime")
	FVector GetCurrentMoveTarget() const { return CurrentMoveTarget; }

	UFUNCTION(BlueprintPure, Category="Creature|Runtime")
	bool HasActiveMoveTarget() const { return bHasActiveMoveTarget; }

protected:
	AAIController* GetCreatureAIController() const;
	AAIController* EnsureCreatureAIController();
	bool IsCloseToCurrentMoveTarget(float AcceptanceRadius) const;
	bool CanRequestMoveTo(const FVector& TargetLocation) const;
	void DrawThreatDebug(AActor* ThreatActor, bool bInRange, bool bInCone);
	void DebugMessage(const FString& Message, const FColor& Color = FColor::Cyan, float Duration = 2.0f) const;
	bool CanPrintDebugStatus();
};
