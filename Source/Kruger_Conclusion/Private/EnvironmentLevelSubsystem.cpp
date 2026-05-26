#include "EnvironmentLevelSubsystem.h"
#include "EcoGameInstanceSubsystem.h"

void UEnvironmentLevelSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Reset counters when a new world loads
    TotalPoachers = 0;
    PoachersCaptured = 0;
    PoachersEscaped = 0;
    TotalAnimals = 0;
    AnimalsAlive = 0;
}

void UEnvironmentLevelSubsystem::RegisterPoacher() { TotalPoachers++; }
void UEnvironmentLevelSubsystem::RegisterAnimal() { TotalAnimals++; AnimalsAlive++; }

void UEnvironmentLevelSubsystem::OnPoacherCaptured()
{
    PoachersCaptured++;

    // Handshake: Give persistent rewards to the GameInstance Wallet. [TO BE IMPLEMENTED]

    //if (UGameInstance* GI = GetWorld()->GetGameInstance())
    //{
    //    if (UEcoGameInstanceSubsystem* EcoWallet = GI->GetSubsystem<UEcoGameInstanceSubsystem>())
    //    {
    //        EcoWallet->AddCredits(RewardPerCapture);
    //    }
    //}

    CheckWinCondition();
}

void UEnvironmentLevelSubsystem::OnPoacherEscaped()
{
    PoachersEscaped++;
    CheckWinCondition();
}

void UEnvironmentLevelSubsystem::OnAnimalPoached()
{
    if (AnimalsAlive > 0) AnimalsAlive--;
	// Add a 'lose level' if too many animals die here. [TO BE IMPLEMENTED]
}

void UEnvironmentLevelSubsystem::CheckWinCondition()
{
    if (TotalPoachers == 0) return;

    // Add up everyone we've dealt with so far.
    int32 AccountedPoachers = PoachersCaptured + PoachersEscaped;

    // Only proceed if there are zero active poachers left running around (level finish).
    if (AccountedPoachers == TotalPoachers)
    {
        // Now calculate the final score of the round
        float FinalCaptureRate = (float)PoachersCaptured / (float)TotalPoachers;

        if (FinalCaptureRate >= WinThresholdPercentage)
        {
            OnLevelWon.Broadcast();
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("LEVEL WIN: All poachers processed. >= 50% caught!"));
        }
        else
        {
            OnLevelLost.Broadcast(); // Triggered because they ran out of poachers and failed the quota.
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("LEVEL LOST: Out of poachers, and quota not met."));
        }
    }
    else
    {
        // Debug message to see how many are left during gameplay
        int32 ActivePoachersRemaining = TotalPoachers - AccountedPoachers;
        UE_LOG(LogTemp, Log, TEXT("Poachers remaining active on map: %d"), ActivePoachersRemaining);
    }
}