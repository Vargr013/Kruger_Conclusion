#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/PPGameTypes.h"
#include "PPAnimalCharacter.generated.h"

class UPPHealthComponent;

UCLASS()
class KRUGER_CONCLUSION_API APPAnimalCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APPAnimalCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPPHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EAnimalState CurrentState = EAnimalState::Wandering;

	UFUNCTION()
	void HandleDeath();

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAnimalState(EAnimalState NewState);

	UFUNCTION(BlueprintPure, Category = "AI")
	EAnimalState GetAnimalState() const { return CurrentState; }
};
