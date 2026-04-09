#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/PPGameTypes.h"
#include "PPPoacherCharacter.generated.h"

class AActor;

UCLASS()
class KRUGER_CONCLUSION_API APPPoacherCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APPPoacherCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EPoacherState CurrentState = EPoacherState::Waiting;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	AActor* CurrentTargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	AActor* CurrentThreatActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poacher")
	bool bCaptured = false;

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetPoacherState(EPoacherState NewState);

	UFUNCTION(BlueprintPure, Category = "AI")
	EPoacherState GetPoacherState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetTargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetTargetActor() const { return CurrentTargetActor; }

	UFUNCTION(BlueprintPure, Category = "AI")
	bool HasTarget() const { return CurrentTargetActor != nullptr; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetThreatActor(AActor* NewThreat);

	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetThreatActor() const { return CurrentThreatActor; }

	UFUNCTION(BlueprintPure, Category = "AI")
	bool HasThreat() const { return CurrentThreatActor != nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Poacher")
	void CapturePoacher();

	UFUNCTION(BlueprintPure, Category = "Poacher")
	bool IsCaptured() const { return bCaptured; }
};
