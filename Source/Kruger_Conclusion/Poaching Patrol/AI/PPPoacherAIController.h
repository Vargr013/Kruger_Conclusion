#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PPPoacherAIController.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API APPPoacherAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;
};
