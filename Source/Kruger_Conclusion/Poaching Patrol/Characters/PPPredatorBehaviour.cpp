#include "Characters/PPAnimalCharacter.h"
#include "Data/PPAudioSubsystem.h"
#include "EnvironmentLevelSubsystem.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Actors/PPAnimalHerd.h"

void APPAnimalCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Herd.IsValid()) Herd->RemoveMember(this);
	CancelPredatorAttack();
	UPPAudioSubsystem::StopPredator(this);
	Super::EndPlay(Reason);
}

void APPAnimalCharacter::StopAIUpdates()
{
	CancelPredatorAttack();
	UPPAudioSubsystem::StopPredator(this);
	Super::StopAIUpdates();
}

void APPAnimalCharacter::CancelPredatorAttack()
{
	bPredatorAttackPending = false;
	if (GetWorld()) GetWorldTimerManager().ClearTimer(PredatorAttackTimer);
}

void APPAnimalCharacter::UpdatePredatorBehaviour()
{
	const UEnvironmentLevelSubsystem* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>();
	if (bPoached || IsActorHealthDepleted(this) || (Rules && (Rules->HasRoundEnded() || Rules->IsTutorialMode())))
	{
		StopAIUpdates();
		StopMovement();
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	AActor* Player = FindPlayerActor();
	if (Player && !IsActorHealthDepleted(Player) && Now - LastGrowlTime >= 8.0f
		&& FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(1200.0f))
	{
		UPPAudioSubsystem::PlayPredator(this, false);
		LastGrowlTime = Now;
	}
	AActor* Target = RememberedTarget.Get();
	FPPThreatObservation Observation;
	bool bObserved = CanAttackTarget(Target) && ObserveTarget(Target, Observation);
	if (!bObserved)
	{
		if (AActor* Candidate = FindBestAttackTarget())
		{
			if (Candidate != Target) CancelPredatorAttack();
			Target = Candidate;
			bObserved = ObserveTarget(Target, Observation);
		}
	}
	if (bObserved && FVector::DistSquared(HomeLocation, Observation.Location) <= FMath::Square(PredatorLeashDistance))
	{
		RememberObservation(Target, Observation);
		SetCreatureMoveSpeed(PursuitSpeed);
		if (Observation.bSeen)
		{
			SetAnimalState(EPPAnimalState::Chasing);
			if (IsTargetInAttackRange(Target))
			{
				StopMovement();
				if (!bPredatorAttackPending && Now - LastAttackTime >= AttackCooldown)
				{
					bPredatorAttackPending = true;
					LastAttackTime = Now;
					FVector Facing = Observation.Location - GetActorLocation();
					Facing.Z = 0;
					SetActorRotation(Facing.Rotation());
					SetAnimalState(EPPAnimalState::Attacking);
					UPPAudioSubsystem::PlayPredator(this, true);
					GetWorldTimerManager().SetTimer(PredatorAttackTimer, this, &APPAnimalCharacter::ResolvePredatorAttack, FMath::Max(0.01f, PredatorAttackWindup), false);
				}
			}
			else
			{
				CancelPredatorAttack();
				if (!MoveToLocation(Observation.Location, AttackAcceptanceRadius)) StartIdle();
			}
			return;
		}
	}
	CancelPredatorAttack();
	if (FollowRememberedTarget(PredatorLeashDistance))
	{
		SetCreatureMoveSpeed(PursuitSpeed);
		SetAnimalState(IsSearchingLastLocation() ? EPPAnimalState::Searching : EPPAnimalState::Investigating);
		return;
	}
	if (RememberedTarget.IsValid() || CurrentAnimalState == EPPAnimalState::Chasing
		|| CurrentAnimalState == EPPAnimalState::Investigating || CurrentAnimalState == EPPAnimalState::Searching
		|| CurrentAnimalState == EPPAnimalState::Attacking) StartIdle();
	if (CurrentAnimalState == EPPAnimalState::Idle)
	{
		if (Now >= IdleEndTime) StartRoaming();
		else UpdateIdleLocalWander(Now);
	}
	else if (CurrentAnimalState == EPPAnimalState::Roaming && IsCloseToCurrentMoveTarget(RoamAcceptanceRadius)) StartIdle();
}

void APPAnimalCharacter::ResolvePredatorAttack()
{
	AActor* Target = RememberedTarget.Get();
	const UEnvironmentLevelSubsystem* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>();
	const bool bCanHit = bPredatorAttackPending && bIsPredator && !bPoached && !IsActorHealthDepleted(this)
		&& (!Rules || (!Rules->HasRoundEnded() && !Rules->IsTutorialMode()))
		&& CanAttackTarget(Target) && IsTargetInAttackRange(Target) && CanSeeTarget(Target);
	bPredatorAttackPending = false;
	if (bCanHit) UGameplayStatics::ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass());
}
