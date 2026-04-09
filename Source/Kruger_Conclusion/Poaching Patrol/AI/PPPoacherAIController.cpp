#include "AI/PPPoacherAIController.h"
#include "Characters/PPPoacherCharacter.h"
#include "NavigationSystem.h"

void APPPoacherAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

APPPoacherCharacter* APPPoacherAIController::GetPoacherCharacter() const
{
	return Cast<APPPoacherCharacter>(GetPawn());
}

bool APPPoacherAIController::MoveToTargetActor(float AcceptanceRadius)
{
	APPPoacherCharacter* Poacher = GetPoacherCharacter();
	if (!Poacher || !Poacher->HasTarget())
	{
		return false;
	}

	MoveToActor(Poacher->GetTargetActor(), AcceptanceRadius);
	return true;
}

bool APPPoacherAIController::MoveAwayFromThreat(float Distance)
{
	APPPoacherCharacter* Poacher = GetPoacherCharacter();
	if (!Poacher || !Poacher->HasThreat())
	{
		return false;
	}

	FVector AwayDirection = Poacher->GetActorLocation() - Poacher->GetThreatActor()->GetActorLocation();
	AwayDirection.Z = 0.0f;
	AwayDirection.Normalize();

	FVector RawLocation = Poacher->GetActorLocation() + (AwayDirection * Distance);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return false;

	FNavLocation NavLocation;
	if (NavSys->ProjectPointToNavigation(RawLocation, NavLocation))
	{
		MoveToLocation(NavLocation.Location);
		return true;
	}

	return false;
}

void APPPoacherAIController::StopAIMovement()
{
	StopMovement();
}
