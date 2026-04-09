#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PPAnimalAIController.generated.h"

struct FNavLocation;
class APPAnimalCharacter;

UCLASS()
class KRUGER_CONCLUSION_API APPAnimalAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;

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
};
