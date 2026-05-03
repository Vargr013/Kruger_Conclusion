#include "Characters/PPPoacherCharacter.h"

APPPoacherCharacter::APPPoacherCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	WalkSpeed = 190.0f;
	FleeSpeed = 460.0f;
	FleeDistance = 1400.0f;
}

void APPPoacherCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartDisguisedRoaming();
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
		StartFleeing(PlayerActor);
		return;
	}

	if (CurrentPoacherState == EPPPoacherState::Fleeing)
	{
		if (!ShouldFleeFromThreat(CurrentThreatActor))
		{
			StartDisguisedRoaming();
		}
		else
		{
			MoveToLocation(GetFleeDestination(CurrentThreatActor), RoamAcceptanceRadius);
		}
		return;
	}

	if (CurrentPoacherState == EPPPoacherState::DisguisedRoaming && IsCloseToCurrentMoveTarget(RoamAcceptanceRadius))
	{
		StartDisguisedRoaming();
	}
}

void APPPoacherCharacter::SetPoacherState(EPPPoacherState NewState)
{
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
		MoveToLocation(RoamLocation, RoamAcceptanceRadius);
	}
}

void APPPoacherCharacter::StartFleeing(AActor* ThreatActor)
{
	if (bIsCaptured || !ThreatActor)
	{
		return;
	}

	EnterFleeState(ThreatActor);
	SetPoacherState(EPPPoacherState::Fleeing);
	MoveToLocation(GetFleeDestination(ThreatActor), RoamAcceptanceRadius);
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
