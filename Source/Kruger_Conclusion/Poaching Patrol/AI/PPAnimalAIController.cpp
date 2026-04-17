#include "AI/PPAnimalAIController.h"
#include "Characters/PPAnimalCharacter.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

void APPAnimalAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ScheduleNextWander();
}

void APPAnimalAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(WanderTimerHandle);
	Super::OnUnPossess();
}

void APPAnimalAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (!bAutoWanderEnabled || Result.IsFailure())
	{
		return;
	}

	APPAnimalCharacter* Animal = GetAnimalCharacter();
	if (Animal && Animal->GetAnimalState() == EAnimalState::Wandering && !Animal->HasThreat())
	{
		ScheduleNextWander();
	}
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
	APPAnimalCharacter* Animal = GetAnimalCharacter();
	if (!Animal || Animal->GetAnimalState() != EAnimalState::Wandering || Animal->HasThreat())
	{
		return false;
	}

	FVector WanderLocation;
	if (!GetRandomWanderLocation(WanderLocation, Radius))
	{
		return false;
	}

	const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(WanderLocation);
	return MoveResult != EPathFollowingRequestResult::Failed;
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
	GetWorldTimerManager().ClearTimer(WanderTimerHandle);
	StopMovement();
}

void APPAnimalAIController::ScheduleNextWander()
{
	if (!bAutoWanderEnabled || !GetPawn())
	{
		return;
	}

	float Delay = MinWanderDelay;
	if (MaxWanderDelay > MinWanderDelay)
	{
		Delay = FMath::FRandRange(MinWanderDelay, MaxWanderDelay);
	}

	GetWorldTimerManager().SetTimer(
		WanderTimerHandle,
		this,
		&APPAnimalAIController::TryWander,
		Delay,
		false);
}

void APPAnimalAIController::TryWander()
{
	if (!MoveToWanderLocation(WanderRadius))
	{
		ScheduleNextWander();
	}
}
