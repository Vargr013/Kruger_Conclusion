#include "Characters/PPPoacherCharacter.h"

APPPoacherCharacter::APPPoacherCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	WalkSpeed = 260.0f;
	FleeSpeed = 460.0f;
	FleeDistance = 1400.0f;
}

void APPPoacherCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartDisguisedIdle();
}

void APPPoacherCharacter::UpdateCreatureAI()
{
	if (CurrentPoacherState == EPPPoacherState::Arrested || CurrentPoacherState == EPPPoacherState::Escaped)
	{
		return;
	}

	if (bIsCaptured || CurrentPoacherState == EPPPoacherState::Captured || CurrentPoacherState == EPPPoacherState::FollowingPlayer)
	{
		FollowCaptor();
		return;
	}

	AActor* PlayerActor = FindPlayerActor();
	if (PlayerActor && ShouldFleeFromThreat(PlayerActor))
	{
		if (CurrentPoacherState != EPPPoacherState::Fleeing || CurrentThreatActor != PlayerActor)
		{
			StartFleeing(PlayerActor);
		}
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (CurrentPoacherState == EPPPoacherState::Fleeing)
	{
		if (CurrentTime >= FleeEndTime || !ShouldFleeFromThreat(CurrentThreatActor))
		{
			StartDisguisedIdle();
		}
		else
		{
			MoveToLocation(GetFleeDestination(CurrentThreatActor), RoamAcceptanceRadius);
		}
		return;
	}

	if (CurrentPoacherState == EPPPoacherState::Alert)
	{
		if (CurrentTime >= IdleEndTime)
		{
			StartDisguisedRoaming();
		}
		return;
	}

	if (CurrentPoacherState == EPPPoacherState::DisguisedRoaming && IsCloseToCurrentMoveTarget(RoamAcceptanceRadius))
	{
		StartDisguisedIdle();
	}
}

void APPPoacherCharacter::SetPoacherState(EPPPoacherState NewState)
{
	if (CurrentPoacherState == NewState)
	{
		return;
	}

	CurrentPoacherState = NewState;

	if (bDrawDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s poacher state changed"), *GetName());
	}
}

void APPPoacherCharacter::StartDisguisedRoaming()
{
	if (bIsCaptured)
	{
		return;
	}

	EnterRoamState();
	SetPoacherState(EPPPoacherState::DisguisedRoaming);

	FVector RoamLocation;
	if (GetRandomRoamLocation(RoamLocation))
	{
		if (!MoveToLocation(RoamLocation, RoamAcceptanceRadius))
		{
			StartDisguisedIdle();
		}
	}
	else
	{
		StartDisguisedIdle();
	}
}

void APPPoacherCharacter::StartDisguisedIdle()
{
	if (bIsCaptured)
	{
		return;
	}

	StopMovement();
	EnterRoamState();
	SetPoacherState(EPPPoacherState::Alert);

	const float IdleDuration = FMath::FRandRange(IdleTimeMin, IdleTimeMax);
	IdleEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + IdleDuration : 0.0f;
}

void APPPoacherCharacter::StartFleeing(AActor* ThreatActor)
{
	if (bIsCaptured || !ThreatActor)
	{
		return;
	}

	EnterFleeState(ThreatActor);
	SetPoacherState(EPPPoacherState::Fleeing);
	FleeEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FleeDuration : 0.0f;

	if (!MoveToLocation(GetFleeDestination(ThreatActor), RoamAcceptanceRadius))
	{
		StartDisguisedIdle();
	}
}

void APPPoacherCharacter::CapturePoacher(AActor* NewCaptor)
{
	if (CurrentPoacherState == EPPPoacherState::Arrested || !NewCaptor)
	{
		return;
	}

	bIsCaptured = true;
	CaptorActor = NewCaptor;
	CurrentThreatActor = nullptr;
	SetCreatureMoveSpeed(CapturedMoveSpeed);
	StopMovement();
	SetPoacherState(EPPPoacherState::FollowingPlayer);

	if (bDrawDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s captured by %s"), *GetName(), *NewCaptor->GetName());
	}
}

void APPPoacherCharacter::FollowCaptor()
{
	if (!CaptorActor || CurrentPoacherState == EPPPoacherState::Arrested)
	{
		StopMovement();
		return;
	}

	SetPoacherState(EPPPoacherState::FollowingPlayer);

	const float DistanceToCaptor = FVector::Dist(GetActorLocation(), CaptorActor->GetActorLocation());
	if (DistanceToCaptor > FollowDistance)
	{
		MoveToLocation(CaptorActor->GetActorLocation(), FollowAcceptanceRadius);
	}
	else
	{
		StopMovement();
	}
}

void APPPoacherCharacter::MarkArrested()
{
	bIsCaptured = true;
	SetPoacherState(EPPPoacherState::Arrested);
	StopMovement();
}

void APPPoacherCharacter::Interact_Implementation(AActor* Interactor)
{
	if (!bIsCaptured && CurrentPoacherState != EPPPoacherState::Arrested)
	{
		CapturePoacher(Interactor);
	}
}

FText APPPoacherCharacter::GetInteractionPrompt_Implementation() const
{
	return bIsCaptured
		? FText::FromString(TEXT("Captured"))
		: FText::FromString(TEXT("Capture Poacher"));
}

void APPPoacherCharacter::SetTargetActor(AActor* NewTarget)
{
	CurrentTargetActor = NewTarget;
}

void APPPoacherCharacter::SetThreatActor(AActor* NewThreat)
{
	CurrentThreatActor = NewThreat;
}
