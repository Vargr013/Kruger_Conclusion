#include "Characters/PPAnimalCharacter.h"

namespace
{
const TCHAR* GetAnimalStateName(EPPAnimalState State)
{
	switch (State)
	{
	case EPPAnimalState::Idle:
		return TEXT("Idle");
	case EPPAnimalState::Roaming:
		return TEXT("Roaming");
	case EPPAnimalState::Alert:
		return TEXT("Alert");
	case EPPAnimalState::Fleeing:
		return TEXT("Fleeing");
	default:
		return TEXT("Unknown");
	}
}
}

APPAnimalCharacter::APPAnimalCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APPAnimalCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartIdle();
}

void APPAnimalCharacter::UpdateCreatureAI()
{
	AActor* PlayerActor = FindPlayerActor();
	if (PlayerActor && ShouldFleeFromThreat(PlayerActor))
	{
		if (CurrentAnimalState != EPPAnimalState::Fleeing || CurrentThreatActor != PlayerActor)
		{
			StartFleeing(PlayerActor);
		}
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (CurrentAnimalState == EPPAnimalState::Fleeing)
	{
		if (CurrentTime >= FleeEndTime || !ShouldFleeFromThreat(CurrentThreatActor))
		{
			StartIdle();
		}
		return;
	}

	if (CurrentAnimalState == EPPAnimalState::Idle)
	{
		if (CurrentTime >= IdleEndTime)
		{
			StartRoaming();
		}
		return;
	}

	if (CurrentAnimalState == EPPAnimalState::Roaming && IsCloseToCurrentMoveTarget(RoamAcceptanceRadius))
	{
		StartIdle();
	}
}

void APPAnimalCharacter::SetAnimalState(EPPAnimalState NewState)
{
	if (CurrentAnimalState == NewState)
	{
		return;
	}

	CurrentAnimalState = NewState;

	DebugMessage(FString::Printf(TEXT("Animal state -> %s"), GetAnimalStateName(CurrentAnimalState)), FColor::Green);
}

void APPAnimalCharacter::StartRoaming()
{
	EnterRoamState();
	SetAnimalState(EPPAnimalState::Roaming);

	FVector RoamLocation;
	if (GetRandomRoamLocation(RoamLocation))
	{
		DebugMessage(FString::Printf(TEXT("Roaming to %s"), *RoamLocation.ToCompactString()), FColor::Cyan);
		if (!MoveToLocation(RoamLocation, RoamAcceptanceRadius))
		{
			DebugMessage(TEXT("Roam move failed, returning to idle"), FColor::Red);
			StartIdle();
		}
	}
	else
	{
		DebugMessage(TEXT("Could not find roam location, returning to idle"), FColor::Red);
		StartIdle();
	}
}

void APPAnimalCharacter::StartFleeing(AActor* ThreatActor)
{
	if (!ThreatActor)
	{
		return;
	}

	EnterFleeState(ThreatActor);
	SetAnimalState(EPPAnimalState::Fleeing);
	FleeEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FleeDuration : 0.0f;
	DebugMessage(FString::Printf(TEXT("Fleeing from %s for %.1fs"), *GetNameSafe(ThreatActor), FleeDuration), FColor::Orange);

	if (!MoveToLocation(GetFleeDestination(ThreatActor), RoamAcceptanceRadius))
	{
		DebugMessage(TEXT("Flee move failed, returning to idle"), FColor::Red);
		StartIdle();
	}
}

void APPAnimalCharacter::StartIdle()
{
	StopMovement();
	EnterRoamState();
	SetAnimalState(EPPAnimalState::Idle);

	const float IdleDuration = FMath::FRandRange(IdleTimeMin, IdleTimeMax);
	IdleEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + IdleDuration : 0.0f;
	DebugMessage(FString::Printf(TEXT("Idling for %.1fs"), IdleDuration), FColor::Silver);
}

void APPAnimalCharacter::SetThreatActor(AActor* NewThreat)
{
	CurrentThreatActor = NewThreat;
}

FVector APPAnimalCharacter::GetFleeLocation(float Distance) const
{
	if (!CurrentThreatActor)
	{
		return GetActorLocation();
	}

	const float PreviousFleeDistance = FleeDistance;
	const FVector FleeLocation = GetActorLocation() + (GetDirectionAwayFromActor(CurrentThreatActor) * Distance);
	return Distance == PreviousFleeDistance ? GetFleeDestination(CurrentThreatActor) : FleeLocation;
}
