#include "Characters/PPPoacherCharacter.h"

#include "Characters/PPAnimalCharacter.h"
#include "BaseProjectile.h"
#include "Data/PPHealthComponent.h"
#include <EnvironmentLevelSubsystem.h>
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kruger_ConclusionPlayerController.h"

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
	case EPPPoacherState::EngagingPlayer:
		return TEXT("EngagingPlayer");
	case EPPPoacherState::Subdued:
		return TEXT("Subdued");
	default:
		return TEXT("Unknown");
	}
}
}

APPPoacherCharacter::APPPoacherCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	WalkSpeed = 260.0f;
	FleeSpeed = 800.0f;
	CapturedMoveSpeed = 550.0f;
	FleeDistance = 1400.0f;
	AttackRange = 170.0f;
	AttackDamage = 10.0f;
	AttackCooldown = 1.5f;
	AttackAcceptanceRadius = 130.0f;
}

void APPPoacherCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelPlayerAttack();
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SubdualTimerHandle);
		GetWorldTimerManager().ClearTimer(PepperSprayTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

float APPPoacherCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const bool bPlayerProjectile = IsPlayerProjectileDamage(EventInstigator, DamageCauser);
	if (bPlayerProjectile && bIsSubdued)
	{
		return 0.0f;
	}

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bPlayerProjectile && !bIsCaptured && !bPendingRemovalAfterArrest && HealthComponent)
	{
		const float Threshold = HealthComponent->GetMaxHealth() * FMath::Clamp(ProjectileSubdualHealthFraction, 0.0f, 1.0f);
		if (HealthComponent->GetCurrentHealth() <= Threshold)
		{
			EnterSubduedState(0.0f, true);
		}
	}
	return AppliedDamage;
}

void APPPoacherCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartDisguisedIdle();

	if (UWorld* World = GetWorld())
	{
		if (UEnvironmentLevelSubsystem* LevelSubsystem = World->GetSubsystem<UEnvironmentLevelSubsystem>())
		{
			LevelSubsystem->RegisterPoacher(this);
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

	if (bIsSubdued || CurrentPoacherState == EPPPoacherState::Subdued || bCaptureAttemptInProgress)
	{
		StopMovement();
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (const UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr)
	{
		if (LevelSubsystem->HasRoundEnded())
		{
			CancelPlayerAttack();
			StopMovement();
			return;
		}
	}

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
			TryMoveToFleeDestination(CurrentThreatActor, RoamAcceptanceRadius);
		}
		return;
	}

	AActor* ThreatActor = FindBestThreatActor();
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
			TryMoveToFleeDestination(CurrentThreatActor, RoamAcceptanceRadius);
		}
		return;
	}

	if (ThreatActor)
	{
		DisengageFromPlayer(false);
		CurrentTargetActor = nullptr;
		StartFleeing(ThreatActor);
		return;
	}

	AActor* PlayerActor = FindPlayerActor();
	const bool bPlayerDetected = CanDetectPlayerForCombat(PlayerActor);
	if (bPlayerDetected)
	{
		LastPlayerDetectedTime = CurrentTime;
	}
	const bool bRememberedPlayer = CurrentPoacherState == EPPPoacherState::EngagingPlayer
		&& PlayerActor
		&& CurrentTime - LastPlayerDetectedTime <= PlayerTargetMemoryTime;
	if ((bPlayerDetected || bRememberedPlayer) && IsWithinCombatLeash(PlayerActor) && !IsActorHealthDepleted(PlayerActor))
	{
		StartEngagingPlayer(PlayerActor);
		UpdatePlayerCombat(PlayerActor, CurrentTime);
		return;
	}
	if (CurrentPoacherState == EPPPoacherState::EngagingPlayer)
	{
		DisengageFromPlayer(true);
		return;
	}

	if (AActor* AttackTarget = FindBestAttackTarget())
	{
		CurrentTargetActor = AttackTarget;
		SetPoacherState(EPPPoacherState::DisguisedRoaming);
		TryAttackTarget(AttackTarget);
		return;
	}

	CurrentTargetActor = nullptr;

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

	if (const APPAnimalCharacter* Animal = Cast<APPAnimalCharacter>(PotentialThreat))
	{
		return Animal->IsPredator();
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

	CancelPlayerAttack();
	EnterFleeState(ThreatActor);
	SetPoacherState(EPPPoacherState::Fleeing);
	const float FleeDuration = GetRandomFleeDuration();
	FleeEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FleeDuration : 0.0f;
	DebugMessage(FString::Printf(TEXT("Fleeing from %s for %.1fs"), *GetNameSafe(ThreatActor), FleeDuration), FColor::Orange);

	if (!TryMoveToFleeDestination(ThreatActor, RoamAcceptanceRadius))
	{
		DebugMessage(TEXT("Poacher flee move failed, returning to idle"), FColor::Red);
		StartDisguisedIdle();
	}
}

void APPPoacherCharacter::StartEngagingPlayer(AActor* PlayerActor)
{
	if (!PlayerActor || bIsCaptured || bIsSubdued || bPendingRemovalAfterArrest)
	{
		return;
	}

	const bool bWasAlreadyEngaging = CurrentPoacherState == EPPPoacherState::EngagingPlayer && CurrentTargetActor == PlayerActor;
	CurrentThreatActor = nullptr;
	CurrentTargetActor = PlayerActor;
	SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(CombatMoveSpeed));
	SetPoacherState(EPPPoacherState::EngagingPlayer);
	if (!bWasAlreadyEngaging)
	{
		BroadcastCombatEvent(EPPPoacherCombatEvent::Engaged);
		BP_OnEngagedPlayer(PlayerActor);
	}
}

void APPPoacherCharacter::UpdatePlayerCombat(AActor* PlayerActor, float CurrentTime)
{
	if (!PlayerActor || bPlayerAttackWindupActive)
	{
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), PlayerActor->GetActorLocation());
	const bool bAttackCooldownReady = CurrentTime - LastAttackTime >= AttackCooldown;
	if (!bHasPlayerAttackSlot && bAttackCooldownReady)
	{
		if (UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr)
		{
			bHasPlayerAttackSlot = LevelSubsystem->TryAcquirePlayerAttackSlot(this);
		}
	}

	if (bHasPlayerAttackSlot)
	{
		if (Distance <= AttackRange && HasClearLineOfSightTo(PlayerActor))
		{
			StopMovement();
			TryStartPlayerAttack(PlayerActor, CurrentTime);
		}
		else
		{
			MoveToLocation(PlayerActor->GetActorLocation(), AttackAcceptanceRadius);
		}
		return;
	}

	if (Distance > CombatHoldDistance)
	{
		MoveToLocation(PlayerActor->GetActorLocation(), FMath::Max(AttackAcceptanceRadius, CombatHoldDistance - 50.0f));
	}
	else
	{
		StopMovement();
	}
}

bool APPPoacherCharacter::CanDetectPlayerForCombat(AActor* PlayerActor) const
{
	if (!PlayerActor || IsActorHealthDepleted(PlayerActor))
	{
		return false;
	}

	EPPThreatDetectionType DetectionType = EPPThreatDetectionType::None;
	if (!CanDetectThreat(PlayerActor, DetectionType))
	{
		return false;
	}
	return DetectionType == EPPThreatDetectionType::Sound || HasClearLineOfSightTo(PlayerActor);
}

bool APPPoacherCharacter::HasClearLineOfSightTo(AActor* TargetActor) const
{
	if (!TargetActor || !GetWorld())
	{
		return false;
	}

	FVector Start = GetActorLocation();
	Start.Z += BaseEyeHeight;
	FVector End = TargetActor->GetActorLocation();
	End.Z += 50.0f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PoacherCombatVisibility), false, this);
	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	return !bBlocked || Hit.GetActor() == TargetActor;
}

bool APPPoacherCharacter::IsWithinCombatLeash(AActor* PlayerActor) const
{
	return PlayerActor && FVector::DistSquared(HomeLocation, PlayerActor->GetActorLocation()) <= FMath::Square(FMath::Max(0.0f, CombatLeashDistance));
}

bool APPPoacherCharacter::TryStartPlayerAttack(AActor* PlayerActor, float CurrentTime)
{
	if (!PlayerActor || bPlayerAttackWindupActive || CurrentTime - LastAttackTime < AttackCooldown)
	{
		return false;
	}

	UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr;
	if (!LevelSubsystem)
	{
		return false;
	}
	if (!bHasPlayerAttackSlot)
	{
		bHasPlayerAttackSlot = LevelSubsystem->TryAcquirePlayerAttackSlot(this);
	}
	if (!bHasPlayerAttackSlot)
	{
		return false;
	}
	bPlayerAttackWindupActive = true;
	LastAttackTime = CurrentTime;
	CurrentTargetActor = PlayerActor;
	StopMovement();

	FVector Facing = PlayerActor->GetActorLocation() - GetActorLocation();
	Facing.Z = 0.0f;
	if (Facing.Normalize())
	{
		SetActorRotation(Facing.Rotation());
	}

	BroadcastCombatEvent(EPPPoacherCombatEvent::AttackWindup);
	BP_OnAttackWindup(PlayerActor);
	GetWorldTimerManager().SetTimer(PlayerAttackWindupTimerHandle, this, &APPPoacherCharacter::ResolvePlayerAttack, FMath::Max(0.01f, AttackWindupDuration), false);
	return true;
}

void APPPoacherCharacter::ResolvePlayerAttack()
{
	AActor* PlayerActor = CurrentTargetActor;
	const bool bCanHit = bPlayerAttackWindupActive
		&& CurrentPoacherState == EPPPoacherState::EngagingPlayer
		&& !bIsSubdued
		&& !bIsCaptured
		&& PlayerActor
		&& !IsActorHealthDepleted(PlayerActor)
		&& IsTargetInAttackRange(PlayerActor)
		&& HasClearLineOfSightTo(PlayerActor);

	bPlayerAttackWindupActive = false;
	if (bCanHit)
	{
		UGameplayStatics::ApplyDamage(PlayerActor, AttackDamage, GetController(), this, UDamageType::StaticClass());
		BroadcastCombatEvent(EPPPoacherCombatEvent::AttackHit);
		BP_OnAttackHit(PlayerActor);
	}
	else
	{
		BroadcastCombatEvent(EPPPoacherCombatEvent::AttackMiss);
		BP_OnAttackMiss(PlayerActor);
	}

	if (UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr)
	{
		LevelSubsystem->ReleasePlayerAttackSlot(this);
	}
	bHasPlayerAttackSlot = false;
}

void APPPoacherCharacter::CancelPlayerAttack()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(PlayerAttackWindupTimerHandle);
		if (UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>())
		{
			LevelSubsystem->ReleasePlayerAttackSlot(this);
		}
	}
	bPlayerAttackWindupActive = false;
	bHasPlayerAttackSlot = false;
}

void APPPoacherCharacter::DisengageFromPlayer(bool bReturnToIdle)
{
	const bool bWasEngaging = CurrentPoacherState == EPPPoacherState::EngagingPlayer;
	CancelPlayerAttack();
	CurrentTargetActor = nullptr;
	if (bWasEngaging)
	{
		BroadcastCombatEvent(EPPPoacherCombatEvent::Disengaged);
	}
	if (bReturnToIdle && !bIsCaptured && !bIsSubdued)
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

	CancelPlayerAttack();
	ClearSubduedState(false);
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

bool APPPoacherCharacter::CanStartCaptureAttempt() const
{
	return bIsSubdued
		&& CurrentPoacherState == EPPPoacherState::Subdued
		&& !bIsCaptured
		&& !bCaptureAttemptInProgress
		&& !bPendingRemovalAfterArrest
		&& !bProcessedAsEscaped
		&& CurrentPoacherState != EPPPoacherState::Captured
		&& CurrentPoacherState != EPPPoacherState::FollowingPlayer
		&& CurrentPoacherState != EPPPoacherState::Arrested
		&& CurrentPoacherState != EPPPoacherState::Escaped
		&& !IsCaptureRetryLocked();
}

bool APPPoacherCharacter::BeginCaptureAttempt()
{
	if (!CanStartCaptureAttempt())
	{
		return false;
	}

	bCaptureAttemptInProgress = true;
	CancelPlayerAttack();
	StopMovement();
	DebugMessage(TEXT("Restraint attempt started"), FColor::Yellow, 2.0f);
	return true;
}

bool APPPoacherCharacter::ResolveCaptureAttempt(EPPRestraintResult Result, AActor* AttemptingCaptor)
{
	if (!bCaptureAttemptInProgress)
	{
		return false;
	}

	bCaptureAttemptInProgress = false;
	if (Result == EPPRestraintResult::Success && AttemptingCaptor)
	{
		CapturePoacher(AttemptingCaptor);
		return bIsCaptured;
	}

	const bool bWasProjectileSubdued = bSubduedByProjectile;
	ClearSubduedState(false);
	if (bWasProjectileSubdued && HealthComponent)
	{
		HealthComponent->ResetHealth();
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	CaptureRetryLockedUntil = CurrentTime + FMath::Max(0.0f, CaptureRetryLockDuration);
	if (AttemptingCaptor)
	{
		StartFleeing(AttemptingCaptor);
	}
	else
	{
		StartDisguisedIdle();
	}

	DebugMessage(
		Result == EPPRestraintResult::Cancelled ? TEXT("Restraint cancelled: breaking away") : TEXT("Restraint failed: breaking away"),
		FColor::Red,
		3.0f);
	return true;
}

void APPPoacherCharacter::AbortCaptureAttempt()
{
	bCaptureAttemptInProgress = false;
}

void APPPoacherCharacter::EnterSubduedState(float Duration, bool bFromProjectile)
{
	if (bIsCaptured || bPendingRemovalAfterArrest || CurrentPoacherState == EPPPoacherState::Arrested || CurrentPoacherState == EPPPoacherState::Escaped)
	{
		return;
	}

	CancelPlayerAttack();
	bIsSubdued = true;
	bSubduedByProjectile = bSubduedByProjectile || bFromProjectile;
	CurrentThreatActor = nullptr;
	CurrentTargetActor = nullptr;
	StopMovement();
	SetPoacherState(EPPPoacherState::Subdued);
	BroadcastCombatEvent(EPPPoacherCombatEvent::Subdued);

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SubdualTimerHandle);
		if (!bFromProjectile && Duration > 0.0f)
		{
			GetWorldTimerManager().SetTimer(SubdualTimerHandle, this, &APPPoacherCharacter::RecoverFromTimedSubdual, Duration, false);
		}
	}
}

void APPPoacherCharacter::ClearSubduedState(bool bResumeBehaviour)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SubdualTimerHandle);
	}
	bIsSubdued = false;
	bSubduedByProjectile = false;
	if (bResumeBehaviour && !bIsCaptured && CurrentPoacherState == EPPPoacherState::Subdued)
	{
		StartDisguisedIdle();
	}
}

bool APPPoacherCharacter::IsCaptureRetryLocked() const
{
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return CurrentTime < CaptureRetryLockedUntil;
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

	CancelPlayerAttack();
	ClearSubduedState(false);
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

	CancelPlayerAttack();
	ClearSubduedState(false);
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
		TryMoveToFleeDestination(PreviousCaptor, RoamAcceptanceRadius);
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
	if (CurrentPoacherState == EPPPoacherState::Arrested || bPendingRemovalAfterArrest || bIsCaptured || !GetWorld())
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
	EnterSubduedState(SafeDuration, false);
	BP_OnDisoriented();

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
	if (CurrentPoacherState == EPPPoacherState::EngagingPlayer)
	{
		SetCreatureMoveSpeed(GetAdjustedPoacherMoveSpeed(CombatMoveSpeed));
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

void APPPoacherCharacter::RecoverFromTimedSubdual()
{
	ClearSubduedState(true);
}

void APPPoacherCharacter::Interact_Implementation(AActor* Interactor)
{
	if (!Interactor || !CanStartCaptureAttempt())
	{
		return;
	}

	APawn* InteractorPawn = Cast<APawn>(Interactor);
	AKruger_ConclusionPlayerController* PlayerController = InteractorPawn
		? Cast<AKruger_ConclusionPlayerController>(InteractorPawn->GetController())
		: nullptr;
	if (!PlayerController || !PlayerController->StartPoacherRestraint(this))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not start restraint minigame for %s"), *GetName());
	}
}

FText APPPoacherCharacter::GetInteractionPrompt_Implementation() const
{
	if (CurrentPoacherState == EPPPoacherState::Arrested || bPendingRemovalAfterArrest)
	{
		return FText::FromString(TEXT("Arrested"));
	}

	if (bIsCaptured)
	{
		return FText::FromString(TEXT("Captured"));
	}
	if (bCaptureAttemptInProgress)
	{
		return FText::FromString(TEXT("Restraint In Progress"));
	}
	if (IsCaptureRetryLocked())
	{
		return FText::FromString(TEXT("Poacher Breaking Away"));
	}
	return bIsSubdued
		? FText::FromString(TEXT("Restrain Poacher"))
		: FText::FromString(TEXT("Subdue Poacher First"));
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

	if (const APPAnimalCharacter* DamageCauserAnimal = Cast<APPAnimalCharacter>(LastDamageCauser))
	{
		if (DamageCauserAnimal->IsPredator())
		{
			bProcessedAsEscaped = true;
			bIsCaptured = false;
			bCanEscapeAfterCapture = false;
			EscapeProgress = 0.0f;
			CaptorActor = nullptr;
			CurrentThreatActor = nullptr;
			CurrentTargetActor = nullptr;
			SetPoacherState(EPPPoacherState::Escaped);
			StopMovement();
			StopAIUpdates();
			SetActorEnableCollision(false);

			if (UWorld* World = GetWorld())
			{
				if (UEnvironmentLevelSubsystem* LevelSubsystem = World->GetSubsystem<UEnvironmentLevelSubsystem>())
				{
					LevelSubsystem->ReportPoacherPermanentlyEscaped(this);
				}

				const float SafeDelay = FMath::Max(0.0f, PredatorRemovalDelay);
				if (SafeDelay <= 0.0f)
				{
					FinalizePredatorRemoval();
				}
				else
				{
					GetWorldTimerManager().SetTimer(PredatorRemovalTimerHandle, this, &APPPoacherCharacter::FinalizePredatorRemoval, SafeDelay, false);
				}
			}

			DebugMessage(TEXT("Poacher removed by predator"), FColor::Red, 3.0f);
			return;
		}
	}

	EnterSubduedState(0.0f, true);
	DebugMessage(TEXT("Poacher subdued: restraint required"), FColor::Yellow, 3.0f);
}

bool APPPoacherCharacter::CanAttackTarget(AActor* PotentialTarget) const
{
	if (!Super::CanAttackTarget(PotentialTarget) || bIsCaptured || bPendingRemovalAfterArrest || CurrentPoacherState == EPPPoacherState::Arrested || CurrentPoacherState == EPPPoacherState::Escaped)
	{
		return false;
	}

	const APPAnimalCharacter* Animal = Cast<APPAnimalCharacter>(PotentialTarget);
	return Animal && !Animal->IsPredator();
}

void APPPoacherCharacter::BroadcastCombatEvent(EPPPoacherCombatEvent Event) const
{
	if (UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr)
	{
		LevelSubsystem->BroadcastPoacherCombatEvent(const_cast<APPPoacherCharacter*>(this), Event);
	}
}

bool APPPoacherCharacter::IsPlayerProjectileDamage(AController* EventInstigator, AActor* DamageCauser) const
{
	return EventInstigator
		&& EventInstigator->IsPlayerController()
		&& Cast<ABaseProjectile>(DamageCauser) != nullptr;
}

void APPPoacherCharacter::FinalizePredatorRemoval()
{
	if (!bProcessedAsEscaped)
	{
		return;
	}

	SetActorHiddenInGame(true);
	Destroy();
}
