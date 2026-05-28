#pragma once

#include "CoreMinimal.h"
#include "Characters/PPCreatureBase.h"
#include "Data/PPGameTypes.h"
#include "GameplayTagContainer.h"
#include "PPAnimalCharacter.generated.h"

class AActor;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class KRUGER_CONCLUSION_API APPAnimalCharacter : public APPCreatureBase
{
	GENERATED_BODY()

public:
	APPAnimalCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Animal|Visuals")
	UStaticMeshComponent* StaticAnimalMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Visuals")
	bool bUseStaticAnimalMeshVisual = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Visuals")
	TSoftObjectPtr<UStaticMesh> StaticAnimalMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|State")
	EPPAnimalState CurrentAnimalState = EPPAnimalState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Identity")
	FGameplayTag AnimalSpeciesTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Identity")
	bool bIsPredator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Timing")
	float IdleTimeMin = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Timing")
	float IdleTimeMax = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Health")
	bool bRemoveAfterPoached = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Health", meta=(ClampMin="0.0"))
	float PoachedRemovalDelay = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category="Animal|Health")
	bool bPoached = false;

	float IdleEndTime = 0.0f;
	float FleeEndTime = 0.0f;
	float NextIdleLocalWanderTime = 0.0f;
	FTimerHandle PoachedRemovalTimerHandle;

public:
	virtual void UpdateCreatureAI() override;
	virtual bool IsValidThreatActor_Implementation(AActor* PotentialThreat) const override;

	UFUNCTION(BlueprintCallable, Category="Animal|State")
	void SetAnimalState(EPPAnimalState NewState);

	UFUNCTION(BlueprintPure, Category="Animal|State")
	EPPAnimalState GetAnimalState() const { return CurrentAnimalState; }

	UFUNCTION(BlueprintCallable, Category="Animal|Behaviour")
	void StartRoaming();

	UFUNCTION(BlueprintCallable, Category="Animal|Behaviour")
	void StartFleeing(AActor* ThreatActor);

	UFUNCTION(BlueprintCallable, Category="Animal|Behaviour")
	void StartIdle();

	UFUNCTION(BlueprintCallable, Category="Animal|State")
	void SetThreatActor(AActor* NewThreat);

	UFUNCTION(BlueprintPure, Category="Animal|AI")
	FVector GetFleeLocation(float Distance = 1000.0f) const;

	UFUNCTION(BlueprintPure, Category="Animal|Identity")
	FGameplayTag GetAnimalSpeciesTag() const { return AnimalSpeciesTag; }

	UFUNCTION(BlueprintPure, Category="Animal|Identity")
	bool IsPredator() const { return bIsPredator; }

protected:
	void UpdateIdleLocalWander(float CurrentTime);
	void ApplyAnimalVisualMesh();
	virtual void HandleHealthDepleted() override;
	virtual bool CanAttackTarget(AActor* PotentialTarget) const override;

	UFUNCTION()
	void FinalizePoachedRemoval();
};
