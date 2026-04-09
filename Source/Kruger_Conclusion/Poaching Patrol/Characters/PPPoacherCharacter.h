#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/PPGameTypes.h"
#include "PPPoacherCharacter.generated.h"

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poacher")
	bool bCaptured = false;

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetPoacherState(EPoacherState NewState);

	UFUNCTION(BlueprintPure, Category = "AI")
	EPoacherState GetPoacherState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "Poacher")
	void CapturePoacher();

	UFUNCTION(BlueprintPure, Category = "Poacher")
	bool IsCaptured() const { return bCaptured; }
};
