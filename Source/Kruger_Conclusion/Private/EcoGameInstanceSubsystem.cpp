#include "EcoGameInstanceSubsystem.h"

void UEcoGameInstanceSubsystem::AddCredits(int32 Amount)
{
    TotalCredits += Amount;
    UE_LOG(LogTemp, Log, TEXT("Credits Added! Current Balance: %d"), TotalCredits);
}

bool UEcoGameInstanceSubsystem::TrySpendCredits(int32 Amount)
{
    if (TotalCredits >= Amount)
    {
        TotalCredits -= Amount;
        return true;
    }
    return false;
}