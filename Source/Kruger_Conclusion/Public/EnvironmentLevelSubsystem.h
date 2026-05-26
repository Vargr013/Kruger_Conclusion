#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnvironmentLevelSubsystem.generated.h"

// Dynamic multicast delegate so UI Blueprints can easily listen for the win screen
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelStateChangedSignature);

UCLASS()
class KRUGER_CONCLUSION_API UEnvironmentLevelSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// Should be called automatically by Poachers and Animals when they spawn into the map. Win/Lost condition requires tracking the total number of each in the level.
    void RegisterPoacher();
    void RegisterAnimal();

    // Event hooks for game occurrences
    UFUNCTION(BlueprintCallable, Category = "EcoSystem|LevelRules")
    void OnPoacherCaptured();

    UFUNCTION(BlueprintCallable, Category = "EcoSystem|LevelRules")
    void OnPoacherEscaped();

    UFUNCTION(BlueprintCallable, Category = "EcoSystem|LevelRules")
    void OnAnimalPoached();

    // UI Events (Possible win screen
    UPROPERTY(BlueprintAssignable, Category = "EcoSystem|Events")
    FOnLevelStateChangedSignature OnLevelWon; // Hook for a win screen in the UI.

    UPROPERTY(BlueprintAssignable, Category = "EcoSystem|Events")
    FOnLevelStateChangedSignature OnLevelLost; // Hook for a failure screen in the UI.

private:
    void CheckWinCondition();

    int32 TotalPoachers = 0;
    int32 PoachersCaptured = 0;
    int32 PoachersEscaped = 0;

    int32 TotalAnimals = 0;
    int32 AnimalsAlive = 0;

	const float WinThresholdPercentage = 0.50f; // Win condition: Capture at least 50% of the Poachers in the level. (FUTURE: increase for difficulty scaling)
    //const int32 RewardPerCapture = 100; // Wallet payout (FUTURE STORE SYSTEM)
};