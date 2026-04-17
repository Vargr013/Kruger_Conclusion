#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "PPAnimalAIController.generated.h"

struct FNavLocation;
class APPAnimalCharacter;
struct FAIRequestID;
struct FPathFollowingResult;

UCLASS()
class KRUGER_CONCLUSION_API APPAnimalAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UFUNCTION(BlueprintCallable, Category = "AI")
	APPAnimalCharacter* GetAnimalCharacter() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool GetRandomWanderLocation(FVector& OutLocation, float Radius = 1000.0f) const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool MoveToWanderLocation(float Radius = 1000.0f);

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool MoveToFleeLocation(float Distance = 1200.0f);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopAIMovement();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander")
	bool bAutoWanderEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander", meta = (ClampMin = "100.0"))
	float WanderRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander", meta = (ClampMin = "0.0"))
	float MinWanderDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander", meta = (ClampMin = "0.0"))
	float MaxWanderDelay = 3.5f;

private:
	void ScheduleNextWander();
	void TryWander();

	FTimerHandle WanderTimerHandle;
};
