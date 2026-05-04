#include "Characters/PPCreatureBase.h"

#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

APPCreatureBase::APPCreatureBase()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
}

void APPCreatureBase::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();
	SetCreatureMoveSpeed(WalkSpeed);
	EnsureCreatureAIController();
	StartAIUpdates();
}

void APPCreatureBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAIUpdates();
	Super::EndPlay(EndPlayReason);
}

void APPCreatureBase::StartAIUpdates()
{
	if (!GetWorld() || AIUpdateInterval <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		AIUpdateTimerHandle,
		this,
		&APPCreatureBase::UpdateCreatureAI,
		AIUpdateInterval,
		true);
}

void APPCreatureBase::StopAIUpdates()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(AIUpdateTimerHandle);
	}
}

void APPCreatureBase::UpdateCreatureAI()
{
}

AActor* APPCreatureBase::FindPlayerActor() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const APlayerController* PlayerController = World->GetFirstPlayerController();
	return PlayerController ? PlayerController->GetPawn() : nullptr;
}

bool APPCreatureBase::IsThreatInRange(AActor* ThreatActor) const
{
	if (!ThreatActor)
	{
		return false;
	}

	return FVector::DistSquared(GetActorLocation(), ThreatActor->GetActorLocation()) <= FMath::Square(ThreatDetectionRadius);
}

bool APPCreatureBase::IsThreatInForwardCone(AActor* ThreatActor) const
{
	if (!ThreatActor)
	{
		return false;
	}

	FVector DirectionToThreat = ThreatActor->GetActorLocation() - GetActorLocation();
	DirectionToThreat.Z = 0.0f;
	if (!DirectionToThreat.Normalize())
	{
		return true;
	}

	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		return false;
	}

	const float Dot = FVector::DotProduct(Forward, DirectionToThreat);
	const float HalfConeDegrees = ForwardThreatAngleDegrees * 0.5f;
	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(HalfConeDegrees));
	return Dot >= MinimumDot;
}

bool APPCreatureBase::ShouldFleeFromThreat(AActor* ThreatActor) const
{
	const bool bInRange = IsThreatInRange(ThreatActor);
	const bool bInCone = IsThreatInForwardCone(ThreatActor);
	DrawThreatDebug(ThreatActor, bInRange, bInCone);
	return bInRange && bInCone;
}

FVector APPCreatureBase::GetDirectionAwayFromActor(AActor* ThreatActor) const
{
	if (!ThreatActor)
	{
		return GetActorForwardVector();
	}

	FVector AwayDirection = GetActorLocation() - ThreatActor->GetActorLocation();
	AwayDirection.Z = 0.0f;
	if (!AwayDirection.Normalize())
	{
		return -GetActorForwardVector();
	}

	return AwayDirection;
}

FVector APPCreatureBase::GetFleeDestination(AActor* ThreatActor) const
{
	const FVector RawDestination = GetActorLocation() + (GetDirectionAwayFromActor(ThreatActor) * FleeDistance);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return RawDestination;
	}

	FNavLocation NavLocation;
	return NavSys->ProjectPointToNavigation(RawDestination, NavLocation) ? NavLocation.Location : RawDestination;
}

bool APPCreatureBase::GetRandomRoamLocation(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return false;
	}

	FNavLocation NavLocation;
	if (NavSys->GetRandomReachablePointInRadius(HomeLocation, RoamRadius, NavLocation))
	{
		OutLocation = NavLocation.Location;
		return true;
	}

	return false;
}

bool APPCreatureBase::MoveToLocation(const FVector& TargetLocation, float AcceptanceRadius)
{
	if (!CanRequestMoveTo(TargetLocation))
	{
		return true;
	}

	AAIController* AIController = EnsureCreatureAIController();
	if (!AIController)
	{
		bHasActiveMoveTarget = false;
		return false;
	}

	CurrentMoveTarget = TargetLocation;
	LastMoveRequestTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastMoveRequestTime;
	const EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(TargetLocation, AcceptanceRadius);
	bHasActiveMoveTarget = Result != EPathFollowingRequestResult::Failed;
	return bHasActiveMoveTarget;
}

void APPCreatureBase::StopMovement()
{
	if (AAIController* AIController = GetCreatureAIController())
	{
		AIController->StopMovement();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	bHasActiveMoveTarget = false;
}

void APPCreatureBase::SetCreatureMoveSpeed(float NewSpeed)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = NewSpeed;
	}
}

void APPCreatureBase::EnterFleeState(AActor* ThreatActor)
{
	CurrentThreatActor = ThreatActor;
	SetCreatureMoveSpeed(FleeSpeed);
}

void APPCreatureBase::EnterRoamState()
{
	CurrentThreatActor = nullptr;
	SetCreatureMoveSpeed(WalkSpeed);
}

AAIController* APPCreatureBase::GetCreatureAIController() const
{
	return Cast<AAIController>(GetController());
}

AAIController* APPCreatureBase::EnsureCreatureAIController()
{
	if (!GetController())
	{
		SpawnDefaultController();
	}

	return GetCreatureAIController();
}

bool APPCreatureBase::IsCloseToCurrentMoveTarget(float AcceptanceRadius) const
{
	if (!bHasActiveMoveTarget)
	{
		return true;
	}

	return FVector::DistSquared(GetActorLocation(), CurrentMoveTarget) <= FMath::Square(AcceptanceRadius);
}

bool APPCreatureBase::CanRequestMoveTo(const FVector& TargetLocation) const
{
	if (!bHasActiveMoveTarget || MoveRequestCooldown <= 0.0f)
	{
		return true;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bCooldownFinished = CurrentTime - LastMoveRequestTime >= MoveRequestCooldown;
	const bool bTargetChanged = FVector::DistSquared(CurrentMoveTarget, TargetLocation) > FMath::Square(RoamAcceptanceRadius);
	return bCooldownFinished || bTargetChanged;
}

void APPCreatureBase::DrawThreatDebug(AActor* ThreatActor, bool bInRange, bool bInCone) const
{
	if (!bDrawDebug || !GetWorld())
	{
		return;
	}

	DrawDebugSphere(GetWorld(), GetActorLocation(), ThreatDetectionRadius, 24, bInRange ? FColor::Yellow : FColor::Silver, false, AIUpdateInterval);

	if (ThreatActor)
	{
		DrawDebugLine(
			GetWorld(),
			GetActorLocation(),
			ThreatActor->GetActorLocation(),
			(bInRange && bInCone) ? FColor::Red : FColor::Green,
			false,
			AIUpdateInterval,
			0,
			2.0f);
	}
}
