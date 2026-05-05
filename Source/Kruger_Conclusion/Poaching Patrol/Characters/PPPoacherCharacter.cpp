#include "Characters/PPPoacherCharacter.h"

namespace
{
const TCHAR* GetPoacherStateName(EPPPoacherState State)
{
	switch (State)
	{
	case EPPPoacherState::DisguisedRoaming:
		return TEXT("DisguisedRoaming");
	case EPPPoacherState::Alert:
		return TEXT("Alert");
	case EPPPoacherState::Fleeing:
		return TEXT("Fleeing");
	case EPPPoacherState::Captured:
		return TEXT("Captured");
	case EPPPoacherState::FollowingPlayer:
		return TEXT("FollowingPlayer");
	case EPPPoacherState::Arrested:
		return TEXT("Arrested");
	case EPPPoacherState::Escaped:
		return TEXT("Escaped");
	default:
		return TEXT("Unknown");
	}
}
}

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
	if (CurrentPoacherState == EPPPoacherState::Arrested)
	{
		return;
	}

	if (bIsCaptured || CurrentPoacherState == EPPPoacherState::Captured || CurrentPoacherState == EPPPoacherState::FollowingPlayer)
	{
		UpdateEscapePressure();
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

	if (CurrentPoacherState == EPPPoacherState::Escaped)
	{
		if (CurrentTime >= FleeEndTime || IsCloseToCurrentMoveTarget(RoamAcceptanceRadius))
		{
			DebugMessage(TEXT("Escape burst finished, returning to disguised roaming"), FColor::Green, 2.0f);
			CurrentThreatActor = nullptr;
			StartDisguisedIdle();
		}
		else if (CurrentThreatActor)
		{
			MoveToLocation(GetFleeDestination(CurrentThreatActor), RoamAcceptanceRadius);
		}
		return;
	}

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

	DebugMessage(FString::Printf(TEXT("Poacher state -> %s"), GetPoacherStateName(CurrentPoacherState)), FColor::Green);
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
		DebugMessage(FString::Printf(TEXT("Disguised roaming to %s"), *RoamLocation.ToCompactString()), FColor::Cyan);
		if (!MoveToLocation(RoamLocation, RoamAcceptanceRadius))
		{
			DebugMessage(TEXT("Poacher roam move failed, returning to idle"), FColor::Red);
			StartDisguisedIdle();
		}
	}
	else
	{
		DebugMessage(TEXT("Could not find poacher roam location, returning to idle"), FColor::Red);
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
	DebugMessage(FString::Printf(TEXT("Disguised idle for %.1fs"), IdleDuration), FColor::Silver);
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
	DebugMessage(FString::Printf(TEXT("Fleeing from %s for %.1fs"), *GetNameSafe(ThreatActor), FleeDuration), FColor::Orange);

	if (!MoveToLocation(GetFleeDestination(ThreatActor), RoamAcceptanceRadius))
	{
		DebugMessage(TEXT("Poacher flee move failed, returning to idle"), FColor::Red);
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
	EscapeProgress = 0.0f;
	LastEscapeUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	SetCreatureMoveSpeed(CapturedMoveSpeed);
	StopMovement();
	SetPoacherState(EPPPoacherState::FollowingPlayer);
	DebugMessage(FString::Printf(TEXT("Captured by %s"), *GetNameSafe(NewCaptor)), FColor::Yellow, 3.0f);
}

void APPPoacherCharacter::FollowCaptor()
{
	if (!CaptorActor || CurrentPoacherState == EPPPoacherState::Arrested || CurrentPoacherState == EPPPoacherState::Escaped)
	{
		StopMovement();
		return;
	}

	SetPoacherState(EPPPoacherState::FollowingPlayer);

	const float DistanceToCaptor = FVector::Dist(GetActorLocation(), CaptorActor->GetActorLocation());
	if (CanPrintDebugStatus())
	{
		DebugMessage(FString::Printf(TEXT("Following captor: distance %.0f / %.0f, escape %.1f / %.1f"), DistanceToCaptor, FollowDistance, EscapeProgress, EscapeGraceTime), FColor::Cyan, 1.0f);
	}

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
	bCanEscapeAfterCapture = false;
	EscapeProgress = 0.0f;
	CaptorActor = nullptr;
	CurrentThreatActor = nullptr;
	SetPoacherState(EPPPoacherState::Arrested);
	StopMovement();
	DebugMessage(TEXT("Arrested: escort complete"), FColor::Green, 3.0f);
}

void APPPoacherCharacter::EscapePoacher()
{
	if (CurrentPoacherState == EPPPoacherState::Arrested)
	{
		return;
	}

	AActor* PreviousCaptor = CaptorActor;
	bIsCaptured = false;
	CaptorActor = nullptr;
	EscapeProgress = 0.0f;
	SetCreatureMoveSpeed(FleeSpeed);
	SetPoacherState(EPPPoacherState::Escaped);
	FleeEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FleeDuration : 0.0f;

	if (PreviousCaptor)
	{
		CurrentThreatActor = PreviousCaptor;
		MoveToLocation(GetFleeDestination(PreviousCaptor), RoamAcceptanceRadius);
	}
	else
	{
		StopMovement();
	}

	DebugMessage(TEXT("Escaped capture and fleeing"), FColor::Red, 3.0f);
}

bool APPPoacherCharacter::IsEscortTooFarFromCaptor() const
{
	if (!CaptorActor)
	{
		return false;
	}

	return FVector::DistSquared(GetActorLocation(), CaptorActor->GetActorLocation()) > FMath::Square(EscapeDistance);
}

void APPPoacherCharacter::UpdateEscapePressure()
{
	if (!bCanEscapeAfterCapture || !bIsCaptured || CurrentPoacherState == EPPPoacherState::Arrested || CurrentPoacherState == EPPPoacherState::Escaped)
	{
		EscapeProgress = 0.0f;
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float DeltaTime = LastEscapeUpdateTime > 0.0f ? CurrentTime - LastEscapeUpdateTime : AIUpdateInterval;
	LastEscapeUpdateTime = CurrentTime;

	if (!IsEscortTooFarFromCaptor())
	{
		if (EscapeProgress > 0.0f && bDrawDebug)
		{
			DebugMessage(TEXT("Escape pressure reset: captor close enough"), FColor::Green, 2.0f);
		}

		EscapeProgress = 0.0f;
		return;
	}

	EscapeProgress = FMath::Clamp(EscapeProgress + DeltaTime, 0.0f, EscapeGraceTime);

	if (CanPrintDebugStatus())
	{
		DebugMessage(FString::Printf(TEXT("Escape pressure %.1f / %.1f: captor too far"), EscapeProgress, EscapeGraceTime), FColor::Orange, 1.0f);
	}

	if (EscapeProgress >= EscapeGraceTime)
	{
		EscapePoacher();
	}
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
