#pragma once

#include "CoreMinimal.h"
#include "Characters/PPCreatureBase.h"
#include "Data/PPGameTypes.h"
#include "PPAnimalCharacter.generated.h"

class AActor;

UCLASS()
class KRUGER_CONCLUSION_API APPAnimalCharacter : public APPCreatureBase
{
	GENERATED_BODY()

public:
	APPAnimalCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|State")
	EPPAnimalState CurrentAnimalState = EPPAnimalState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Timing")
	float IdleTimeMin = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Timing")
	float IdleTimeMax = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animal|Flee")
	float FleeDuration = 3.0f;

	float IdleEndTime = 0.0f;
	float FleeEndTime = 0.0f;

public:
	virtual void UpdateCreatureAI() override;

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
};
