#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "PPAnimalAIController.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API APPAnimalAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;

protected:
	FTimerHandle WanderTimer;

	void MoveToRandomLocation();
};
