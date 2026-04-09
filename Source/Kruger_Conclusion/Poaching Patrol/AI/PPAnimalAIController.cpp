#include "AI/PPAnimalAIController.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"

void APPAnimalAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			WanderTimer,
			this,
			&APPAnimalAIController::MoveToRandomLocation,
			3.0f,
			true);
	}
}

void APPAnimalAIController::MoveToRandomLocation()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys)
	{
		return;
	}

	FNavLocation Result;
	const FVector Origin = ControlledPawn->GetActorLocation();
	if (NavSys->GetRandomReachablePointInRadius(Origin, 1000.0f, Result))
	{
		MoveToLocation(Result.Location);
	}
}
