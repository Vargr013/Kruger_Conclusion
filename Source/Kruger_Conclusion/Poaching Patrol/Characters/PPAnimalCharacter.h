#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/PPGameTypes.h"
#include "PPAnimalCharacter.generated.h"

class AActor;
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

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	AActor* CurrentThreatActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bCanStalk = false;

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAnimalState(EAnimalState NewState);

	UFUNCTION(BlueprintPure, Category = "AI")
	EAnimalState GetAnimalState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetThreatActor(AActor* NewThreat);

	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetThreatActor() const { return CurrentThreatActor; }

	UFUNCTION(BlueprintPure, Category = "AI")
	bool HasThreat() const { return CurrentThreatActor != nullptr; }

	UFUNCTION(BlueprintPure, Category = "AI")
	bool CanStalk() const { return bCanStalk; }

	UFUNCTION(BlueprintPure, Category = "AI")
	bool IsHealthLow() const;

	UFUNCTION(BlueprintPure, Category = "AI")
	FVector GetFleeLocation(float Distance = 1000.0f) const;
};
