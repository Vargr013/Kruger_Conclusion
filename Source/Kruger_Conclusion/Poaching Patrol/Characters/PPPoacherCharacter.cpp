#include "Characters/PPPoacherCharacter.h"

#include "Characters/PPAnimalCharacter.h"
#include <EnvironmentLevelSubsystem.h>

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

	if (UWorld* World = GetWorld())
	{
		if (UEnvironmentLevelSubsystem* LevelSubsystem = World->GetSubsystem<UEnvironmentLevelSubsystem>())
		{
			LevelSubsystem->RegisterPoacher();
		}
	}
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

	AActor* ThreatActor = FindBestThreatActor();
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (CurrentPoacherState == EPPPoacherState::Escaped)
	{
		if (CurrentTime >= FleeEndTime)
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
		if (ThreatActor && CurrentThreatActor != ThreatActor)
		{
			StartFleeing(ThreatActor);
			return;
		}

		if (CurrentTime >= FleeEndTime)
		{
			StartDisguisedIdle();
		}
		else
		{
			MoveToLocation(GetFleeDestination(CurrentThreatActor), RoamAcceptanceRadius);
		}
		return;
	}

	if (ThreatActor)
	{
		StartFleeing(ThreatActor);
		return;
	}

	if (CurrentPoacherState == EPPPoacherState::Alert)
	{
		if (CurrentTime >= IdleEndTime)
		{
			StartDisguisedRoaming();
		}
		else
		{
			UpdateIdleLocalWander(CurrentTime);
		}
		return;
	}

	if (CurrentPoacherState == EPPPoacherState::DisguisedRoaming && IsCloseToCurrentMoveTarget(RoamAcceptanceRadius))
	{
		StartDisguisedIdle();
	}
}

bool APPPoacherCharacter::IsValidThreatActor_Implementation(AActor* PotentialThreat) const
{
	if (!Super::IsValidThreatActor_Implementation(PotentialThreat))
	{
		return false;
	}

	if (PotentialThreat == FindPlayerActor())
	{
		return true;
	}

	if (Cast<APPAnimalCharacter>(PotentialThreat))
	{
		return true;
	}

	if (Cast<APPPoacherCharacter>(PotentialThreat))
	{
		if (CanPrintDebugStatus())
		{
			DebugMessage(FString::Printf(TEXT("Ignoring other poacher as threat: %s"), *GetNameSafe(PotentialThreat)), FColor::Green, 1.0f);
		}
		return false;
	}

	return false;
}

void APPPoacherCharacter::EnterFleeState(AActor* ThreatActor)
{
	CurrentThreatActor = ThreatActor;
	SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(FleeSpeed));
}

void APPPoacherCharacter::EnterRoamState()
{
	CurrentThreatActor = nullptr;
	SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(WalkSpeed));
}

void APPPoacherCharacter::SetPoacherState(EPPPoacherState NewState)
{
	if (CurrentPoacherState == NewState)
	{
		return;
	}

	CurrentPoacherState = NewState;
	RefreshPoacherMoveSpeed();

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
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	IdleEndTime = CurrentTime + IdleDuration;
	NextIdleLocalWanderTime = CurrentTime + GetRandomIdleStandDuration();
	DebugMessage(FString::Printf(TEXT("Disguised idle for %.1fs"), IdleDuration), FColor::Silver);
}

void APPPoacherCharacter::UpdateIdleLocalWander(float CurrentTime)
{
	if (CurrentTime < NextIdleLocalWanderTime)
	{
		return;
	}

	if (!bHasActiveMoveTarget || IsCloseToCurrentMoveTarget(IdleLocalWanderAcceptanceRadius))
	{
		FVector LocalWanderLocation;
		if (GetRandomIdleLocalWanderLocation(LocalWanderLocation))
		{
			DebugMessage(FString::Printf(TEXT("Disguised local wander to %s"), *LocalWanderLocation.ToCompactString()), FColor::Silver);
			MoveToLocation(LocalWanderLocation, IdleLocalWanderAcceptanceRadius);
		}
	}

	NextIdleLocalWanderTime = CurrentTime + GetRandomIdleStandDuration();
}

void APPPoacherCharacter::StartFleeing(AActor* ThreatActor)
{
	if (bIsCaptured || !ThreatActor)
	{
		return;
	}

	EnterFleeState(ThreatActor);
	SetPoacherState(EPPPoacherState::Fleeing);
	const float FleeDuration = GetRandomFleeDuration();
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
	SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(CapturedMoveSpeed));
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

bool APPPoacherCharacter::MarkArrested()
{
	if (CurrentPoacherState == EPPPoacherState::Arrested)
	{
		return false;
	}

	bIsCaptured = true;
	bCanEscapeAfterCapture = false;
	EscapeProgress = 0.0f;
	CaptorActor = nullptr;
	CurrentThreatActor = nullptr;
	CurrentTargetActor = nullptr;
	SetPoacherState(EPPPoacherState::Arrested);
	bIsPepperSprayed = false;
	GetWorldTimerManager().ClearTimer(PepperSprayTimerHandle);
	StopMovement();
	StopAIUpdates();
	SetActorEnableCollision(false);
	DebugMessage(TEXT("Arrested: escort complete"), FColor::Green, 3.0f);
	return true;
}

void APPPoacherCharacter::RemoveFromLevelAfterArrest(float Delay)
{
	if (CurrentPoacherState != EPPPoacherState::Arrested)
	{
		return;
	}

	bPendingRemovalAfterArrest = true;

	const float SafeDelay = FMath::Max(0.0f, Delay);
	if (SafeDelay <= 0.0f || !GetWorld())
	{
		FinalizeArrestRemoval();
		return;
	}

	DebugMessage(FString::Printf(TEXT("Removing arrested poacher in %.1fs"), SafeDelay), FColor::Green, SafeDelay);
	GetWorldTimerManager().SetTimer(ArrestRemovalTimerHandle, this, &APPPoacherCharacter::FinalizeArrestRemoval, SafeDelay, false);
}

void APPPoacherCharacter::FinalizeArrestRemoval()
{
	if (CurrentPoacherState != EPPPoacherState::Arrested)
	{
		return;
	}

	bPendingRemovalAfterArrest = false;
	SetActorHiddenInGame(true);
	Destroy();
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
	SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(FleeSpeed));
	SetPoacherState(EPPPoacherState::Escaped);
	const float FleeDuration = GetRandomFleeDuration();
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

	DebugMessage(FString::Printf(TEXT("Escaped capture and fleeing for %.1fs"), FleeDuration), FColor::Red, 3.0f);
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

void APPPoacherCharacter::ApplyPepperSpraySlow(float Duration)
{
	if (CurrentPoacherState == EPPPoacherState::Arrested || bPendingRemovalAfterArrest || !GetWorld())
	{
		return;
	}

	const float RequestedDuration = Duration > 0.0f ? Duration : PepperSprayDuration;
	const float SafeDuration = FMath::Max(0.0f, RequestedDuration);
	if (SafeDuration <= 0.0f)
	{
		return;
	}

	bIsPepperSprayed = true;
	RefreshPoacherMoveSpeed();

	GetWorldTimerManager().ClearTimer(PepperSprayTimerHandle);
	GetWorldTimerManager().SetTimer(PepperSprayTimerHandle, this, &APPPoacherCharacter::ClearPepperSpraySlow, SafeDuration, false);
	DebugMessage(FString::Printf(TEXT("Pepper sprayed: slowed for %.1fs"), SafeDuration), FColor::Purple, 2.0f);
}

float APPPoacherCharacter::GetAdjustedPoacherMoveSpeed(float BaseSpeed) const
{
	if (!bIsPepperSprayed)
	{
		return BaseSpeed;
	}

	const float SlowedSpeed = BaseSpeed * FMath::Max(0.0f, PepperSpraySpeedMultiplier);
	return FMath::Max(MinimumPepperSprayedSpeed, SlowedSpeed);
}

void APPPoacherCharacter::RefreshPoacherMoveSpeed()
{
	if (CurrentPoacherState == EPPPoacherState::Arrested || bPendingRemovalAfterArrest)
	{
		return;
	}

	if (bIsCaptured || CurrentPoacherState == EPPPoacherState::Captured || CurrentPoacherState == EPPPoacherState::FollowingPlayer)
	{
		SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(CapturedMoveSpeed));
		return;
	}

	if (CurrentPoacherState == EPPPoacherState::Fleeing || CurrentPoacherState == EPPPoacherState::Escaped)
	{
		SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(FleeSpeed));
		return;
	}

	SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(WalkSpeed));
}

void APPPoacherCharacter::ClearPepperSpraySlow()
{
	bIsPepperSprayed = false;
	RefreshPoacherMoveSpeed();
	DebugMessage(TEXT("Pepper spray slow ended"), FColor::Green, 2.0f);
}

void APPPoacherCharacter::Interact_Implementation(AActor* Interactor)
{
	if (!bIsCaptured && CurrentPoacherState != EPPPoacherState::Arrested && !bPendingRemovalAfterArrest)
	{
		CapturePoacher(Interactor);
	}
}

FText APPPoacherCharacter::GetInteractionPrompt_Implementation() const
{
	if (CurrentPoacherState == EPPPoacherState::Arrested || bPendingRemovalAfterArrest)
	{
		return FText::FromString(TEXT("Arrested"));
	}

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

void APPPoacherCharacter::HandleHealthDepleted()
{
	if (CurrentPoacherState == EPPPoacherState::Arrested || bPendingRemovalAfterArrest || bIsCaptured)
	{
		return;
	}

	AActor* CaptorActorFromDamage = LastDamageInstigator ? LastDamageInstigator->GetPawn() : nullptr;
	if (!CaptorActorFromDamage)
	{
		CaptorActorFromDamage = LastDamageCauser ? LastDamageCauser->GetOwner() : nullptr;
	}
	if (!CaptorActorFromDamage)
	{
		CaptorActorFromDamage = LastDamageCauser;
	}
	if (!CaptorActorFromDamage)
	{
		CaptorActorFromDamage = FindPlayerActor();
	}

	if (CaptorActorFromDamage)
	{
		CapturePoacher(CaptorActorFromDamage);
	}
	else
	{
		StopMovement();
		SetPoacherState(EPPPoacherState::Captured);
		DebugMessage(TEXT("Poacher subdued but no captor was found"), FColor::Yellow, 3.0f);
	}
}
