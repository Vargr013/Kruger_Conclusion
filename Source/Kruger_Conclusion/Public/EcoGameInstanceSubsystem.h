// This subsystem manages the player's persistent economy across levels. (STORE SETUP IN FUTURE)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EcoGameInstanceSubsystem.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API UEcoGameInstanceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Adds currency to the player's wallet
    UFUNCTION(BlueprintCallable, Category = "EcoSystem|Economy")
    void AddCredits(int32 Amount);

    // Spends currency if the player has enough
    UFUNCTION(BlueprintCallable, Category = "EcoSystem|Economy")
    bool TrySpendCredits(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "EcoSystem|Economy")
    int32 GetTotalCredits() const { return TotalCredits; }

private:
    UPROPERTY(VisibleAnywhere, Category = "EcoSystem|Economy")
    int32 TotalCredits = 0;

    // FUTURE EXPANSION store inventory array will go continue here.
};