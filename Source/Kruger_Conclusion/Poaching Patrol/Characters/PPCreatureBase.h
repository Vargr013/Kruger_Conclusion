#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "PPCreatureBase.generated.h"

class AAIController;
class AController;
class UPPHealthComponent;

UENUM(BlueprintType)
enum class EPPThreatDetectionType : uint8
{
	None UMETA(DisplayName="None"),
	Sight UMETA(DisplayName="Sight"),
	Sound UMETA(DisplayName="Sound")
};

UCLASS(Abstract)
class KRUGER_CONCLUSION_API APPCreatureBase : public ACharacter
{
	GENERATED_BODY()

public:
	APPCreatureBase();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Creature|Health")
	UPPHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Movement")
	float WalkSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Movement")
	float FleeSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Roaming")
	float RoamRadius = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Roaming")
	float RoamAcceptanceRadius = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Idle")
	float IdleLocalWanderRadius = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Idle")
	float IdleLocalWanderAcceptanceRadius = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Idle")
	float IdleStandTimeMin = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Idle")
	float IdleStandTimeMax = 2.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float ThreatDetectionRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float ForwardThreatAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float SightThreatRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float SightThreatAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float SoundThreatRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	bool bUseSightThreats = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	bool bUseSoundThreats = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Threat")
	float FleeDistance = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Flee")
	float FleeDurationMin = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Flee")
	float FleeDurationMax = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Timing")
	float AIUpdateInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Timing")
	float MoveRequestCooldown = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Attack")
	float AttackRange = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Attack")
	float AttackDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Attack")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Attack")
	float AttackAcceptanceRadius = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Movement")
	int32 MaxConsecutiveMoveFailures = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	bool bDebugOnScreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	float DebugStatusInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	float DebugDrawDuration = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	float DebugLineThickness = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	float DebugVerticalOffset = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Creature|Debug")
	int32 DebugSphereSegments = 32;

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

	UPROPERTY(BlueprintReadOnly, Category="Creature|Runtime")
	int32 ConsecutiveMoveFailures = 0;

	UPROPERTY(BlueprintReadOnly, Category="Creature|Attack")
	float LastAttackTime = -1000.0f;

	mutable float LastDebugStatusTime = -1000.0f;

	FTimerHandle AIUpdateTimerHandle;

	UPROPERTY(BlueprintReadOnly, Category="Creature|Health")
	AController* LastDamageInstigator = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Creature|Health")
	AActor* LastDamageCauser = nullptr;

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

	UFUNCTION(BlueprintCallable, Category="Creature|Threat")
	AActor* FindBestThreatActor();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Creature|Threat")
	bool IsValidThreatActor(AActor* PotentialThreat) const;

	UFUNCTION(BlueprintCallable, Category="Creature|Threat")
	bool CanDetectThreat(AActor* PotentialThreat, EPPThreatDetectionType& OutDetectionType) const;

	UFUNCTION(BlueprintPure, Category="Creature|Movement")
	FVector GetDirectionAwayFromActor(AActor* ThreatActor) const;

	UFUNCTION(BlueprintPure, Category="Creature|Movement")
	FVector GetFleeDestination(AActor* ThreatActor) const;

	UFUNCTION(BlueprintCallable, Category="Creature|Movement")
	bool TryMoveToFleeDestination(AActor* ThreatActor, float AcceptanceRadius);

	UFUNCTION(BlueprintPure, Category="Creature|Flee")
	float GetRandomFleeDuration() const;

	UFUNCTION(BlueprintCallable, Category="Creature|Movement")
	bool GetRandomRoamLocation(FVector& OutLocation) const;

	UFUNCTION(BlueprintCallable, Category="Creature|Idle")
	bool GetRandomIdleLocalWanderLocation(FVector& OutLocation) const;

	UFUNCTION(BlueprintPure, Category="Creature|Idle")
	float GetRandomIdleStandDuration() const;

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

	UFUNCTION(BlueprintPure, Category="Creature|Health")
	UPPHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	UFUNCTION()
	void OnHealthDepleted();

	virtual void HandleHealthDepleted();
	virtual bool CanAttackTarget(AActor* PotentialTarget) const;
	AActor* FindBestAttackTarget() const;
	bool IsTargetInAttackRange(AActor* TargetActor) const;
	bool TryAttackTarget(AActor* TargetActor);
	bool MoveTowardAttackTarget(AActor* TargetActor);
	bool IsActorHealthDepleted(AActor* Actor) const;

	AAIController* GetCreatureAIController() const;
	AAIController* EnsureCreatureAIController();
	bool IsCloseToCurrentMoveTarget(float AcceptanceRadius) const;
	bool CanRequestMoveTo(const FVector& TargetLocation) const;
	void RegisterMoveRequestFailure(const FVector& FailedTargetLocation);
	void ResetMoveRequestFailures();
	void DrawThreatDebug(AActor* ThreatActor, bool bInRange, bool bInCone);
	void DebugMessage(const FString& Message, const FColor& Color = FColor::Cyan, float Duration = 2.0f) const;
	bool CanPrintDebugStatus() const;
	FString GetThreatDetectionName(EPPThreatDetectionType DetectionType) const;
};
