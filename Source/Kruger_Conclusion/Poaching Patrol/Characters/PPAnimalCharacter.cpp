#include "Characters/PPAnimalCharacter.h"

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

	if (bDrawDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s animal state changed"), *GetName());
	}
}

void APPAnimalCharacter::StartRoaming()
{
	EnterRoamState();
	SetAnimalState(EPPAnimalState::Roaming);

	FVector RoamLocation;
	if (GetRandomRoamLocation(RoamLocation))
	{
		if (!MoveToLocation(RoamLocation, RoamAcceptanceRadius))
		{
			StartIdle();
		}
	}
	else
	{
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

	if (!MoveToLocation(GetFleeDestination(ThreatActor), RoamAcceptanceRadius))
	{
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
