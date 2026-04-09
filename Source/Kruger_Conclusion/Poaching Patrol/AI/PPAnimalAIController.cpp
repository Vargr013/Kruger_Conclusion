#include "AI/PPAnimalAIController.h"
#include "Characters/PPAnimalCharacter.h"
#include "NavigationSystem.h"

void APPAnimalAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

APPAnimalCharacter* APPAnimalAIController::GetAnimalCharacter() const
{
	return Cast<APPAnimalCharacter>(GetPawn());
}

bool APPAnimalAIController::GetRandomWanderLocation(FVector& OutLocation, float Radius) const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return false;
	}

	FNavLocation Result;
	if (NavSys->GetRandomReachablePointInRadius(ControlledPawn->GetActorLocation(), Radius, Result))
	{
		OutLocation = Result.Location;
		return true;
	}

	return false;
}

bool APPAnimalAIController::MoveToWanderLocation(float Radius)
{
	FVector WanderLocation;
	if (!GetRandomWanderLocation(WanderLocation, Radius))
	{
		return false;
	}

	MoveToLocation(WanderLocation);
	return true;
}

bool APPAnimalAIController::MoveToFleeLocation(float Distance)
{
	APPAnimalCharacter* Animal = GetAnimalCharacter();
	if (!Animal || !Animal->HasThreat())
	{
		return false;
	}

	FVector RawFleeLocation = Animal->GetFleeLocation(Distance);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return false;

	FNavLocation NavLocation;
	if (NavSys->ProjectPointToNavigation(RawFleeLocation, NavLocation))
	{
		MoveToLocation(NavLocation.Location);
		return true;
	}

	return false;
}

void APPAnimalAIController::StopAIMovement()
{
	StopMovement();
}
