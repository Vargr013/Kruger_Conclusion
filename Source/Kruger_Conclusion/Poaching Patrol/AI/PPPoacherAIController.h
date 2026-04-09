#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PPPoacherAIController.generated.h"

class APPPoacherCharacter;

UCLASS()
class KRUGER_CONCLUSION_API APPPoacherAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintCallable, Category = "AI")
	APPPoacherCharacter* GetPoacherCharacter() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool MoveToTargetActor(float AcceptanceRadius = 150.0f);

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool MoveAwayFromThreat(float Distance = 1200.0f);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopAIMovement();
};
