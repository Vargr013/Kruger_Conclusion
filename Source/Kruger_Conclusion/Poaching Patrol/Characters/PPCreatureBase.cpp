#include "Characters/PPCreatureBase.h"

#include "AIController.h"
#include "Data/PPHealthComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

APPCreatureBase::APPCreatureBase()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	HealthComponent = CreateDefaultSubobject<UPPHealthComponent>(TEXT("HealthComponent"));
}

void APPCreatureBase::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &APPCreatureBase::OnHealthDepleted);
	}

	HomeLocation = GetActorLocation();
	SetCreatureMoveSpeed(WalkSpeed);
	EnsureCreatureAIController();
	StartAIUpdates();
	DebugMessage(FString::Printf(TEXT("AI started at home %s"), *HomeLocation.ToCompactString()));
}

void APPCreatureBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAIUpdates();
	Super::EndPlay(EndPlayReason);
}

float APPCreatureBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (DamageAmount <= 0.0f || !HealthComponent || HealthComponent->IsDead())
	{
		return AppliedDamage;
	}

	LastDamageInstigator = EventInstigator;
	LastDamageCauser = DamageCauser;
	HealthComponent->ApplyDamage(DamageAmount);
	return DamageAmount;
}

void APPCreatureBase::OnHealthDepleted()
{
	HandleHealthDepleted();
}

void APPCreatureBase::HandleHealthDepleted()
{
	StopMovement();
	StopAIUpdates();
	SetActorEnableCollision(false);
	DebugMessage(TEXT("Health depleted"), FColor::Red, 3.0f);
}

bool APPCreatureBase::CanAttackTarget(AActor* PotentialTarget) const
{
	return PotentialTarget && PotentialTarget != this && !IsActorHealthDepleted(PotentialTarget);
}

AActor* APPCreatureBase::FindBestAttackTarget() const
{
	AActor* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	auto ConsiderTarget = [this, &BestTarget, &BestDistanceSquared](AActor* Candidate)
	{
		if (!CanAttackTarget(Candidate))
		{
			return;
		}

		EPPThreatDetectionType DetectionType = EPPThreatDetectionType::None;
		if (!CanDetectThreat(Candidate, DetectionType))
		{
			return;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	};

	ConsiderTarget(FindPlayerActor());

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APPCreatureBase> It(World); It; ++It)
		{
			ConsiderTarget(*It);
		}
	}

	return BestTarget;
}

bool APPCreatureBase::IsTargetInAttackRange(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	return FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation()) <= FMath::Square(AttackRange);
}

bool APPCreatureBase::TryAttackTarget(AActor* TargetActor)
{
	if (!CanAttackTarget(TargetActor))
	{
		return false;
	}

	if (!IsTargetInAttackRange(TargetActor))
	{
		return MoveTowardAttackTarget(TargetActor);
	}

	StopMovement();

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (CurrentTime - LastAttackTime < AttackCooldown)
	{
		return true;
	}

	LastAttackTime = CurrentTime;
	UGameplayStatics::ApplyDamage(TargetActor, AttackDamage, GetController(), this, UDamageType::StaticClass());
	DebugMessage(FString::Printf(TEXT("Attacked %s for %.0f damage"), *GetNameSafe(TargetActor), AttackDamage), FColor::Red, 1.5f);
	return true;
}

bool APPCreatureBase::MoveTowardAttackTarget(AActor* TargetActor)
{
	if (!CanAttackTarget(TargetActor))
	{
		return false;
	}

	return MoveToLocation(TargetActor->GetActorLocation(), AttackAcceptanceRadius);
}

bool APPCreatureBase::IsActorHealthDepleted(AActor* Actor) const
{
	if (!Actor)
	{
		return true;
	}

	if (const UPPHealthComponent* ActorHealth = Actor->FindComponentByClass<UPPHealthComponent>())
	{
		return ActorHealth->IsDead();
	}

	return false;
}

void APPCreatureBase::StartAIUpdates()
{
	if (!GetWorld() || AIUpdateInterval <= 0.0f)
	{
		DebugMessage(TEXT("AI updates not started: invalid world or interval"), FColor::Red);
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

	return FVector::DistSquared(GetActorLocation(), ThreatActor->GetActorLocation()) <= FMath::Square(SightThreatRadius);
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
	const float HalfConeDegrees = SightThreatAngleDegrees * 0.5f;
	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(HalfConeDegrees));
	return Dot >= MinimumDot;
}

bool APPCreatureBase::ShouldFleeFromThreat(AActor* ThreatActor) const
{
	EPPThreatDetectionType DetectionType = EPPThreatDetectionType::None;
	const bool bDetected = CanDetectThreat(ThreatActor, DetectionType);
	const bool bInRange = IsThreatInRange(ThreatActor);
	const bool bInCone = IsThreatInForwardCone(ThreatActor);
	const_cast<APPCreatureBase*>(this)->DrawThreatDebug(ThreatActor, bInRange, bInCone);
	return bDetected;
}

AActor* APPCreatureBase::FindBestThreatActor()
{
	AActor* BestThreat = nullptr;
	EPPThreatDetectionType BestDetectionType = EPPThreatDetectionType::None;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	auto ConsiderThreat = [this, &BestThreat, &BestDetectionType, &BestDistanceSquared](AActor* Candidate)
	{
		if (!Candidate || Candidate == this)
		{
			return;
		}

		if (!IsValidThreatActor(Candidate))
		{
			return;
		}

		EPPThreatDetectionType DetectionType = EPPThreatDetectionType::None;
		if (!CanDetectThreat(Candidate, DetectionType))
		{
			return;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestThreat = Candidate;
			BestDetectionType = DetectionType;
		}
	};

	ConsiderThreat(FindPlayerActor());

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APPCreatureBase> It(World); It; ++It)
		{
			ConsiderThreat(*It);
		}
	}

	if (BestThreat && CanPrintDebugStatus())
	{
		DebugMessage(FString::Printf(
			TEXT("Selected threat: %s by %s at %.0fcm"),
			*GetNameSafe(BestThreat),
			*GetThreatDetectionName(BestDetectionType),
			FMath::Sqrt(BestDistanceSquared)),
			FColor::Red,
			1.0f);
	}

	return BestThreat;
}

bool APPCreatureBase::IsValidThreatActor_Implementation(AActor* PotentialThreat) const
{
	return PotentialThreat != nullptr && PotentialThreat != this;
}

bool APPCreatureBase::CanDetectThreat(AActor* PotentialThreat, EPPThreatDetectionType& OutDetectionType) const
{
	OutDetectionType = EPPThreatDetectionType::None;
	if (!PotentialThreat)
	{
		return false;
	}

	const float DistanceSquared = FVector::DistSquared(GetActorLocation(), PotentialThreat->GetActorLocation());

	if (bUseSoundThreats && DistanceSquared <= FMath::Square(SoundThreatRadius))
	{
		OutDetectionType = EPPThreatDetectionType::Sound;
		return true;
	}

	if (bUseSightThreats && DistanceSquared <= FMath::Square(SightThreatRadius) && IsThreatInForwardCone(PotentialThreat))
	{
		OutDetectionType = EPPThreatDetectionType::Sight;
		return true;
	}

	return false;
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

bool APPCreatureBase::TryMoveToFleeDestination(AActor* ThreatActor, float AcceptanceRadius)
{
	if (!ThreatActor)
	{
		return false;
	}

	FVector AwayDirection = GetDirectionAwayFromActor(ThreatActor);
	AwayDirection.Z = 0.0f;
	if (!AwayDirection.Normalize())
	{
		return false;
	}

	TArray<float> CandidateYawOffsets;
	CandidateYawOffsets.Reserve(5);
	CandidateYawOffsets.Add(0.0f);

	const float FirstTurn = FMath::FRandRange(35.0f, 90.0f);
	const float SecondTurn = FMath::FRandRange(35.0f, 90.0f);
	const float FirstSign = FMath::RandBool() ? 1.0f : -1.0f;
	CandidateYawOffsets.Add(FirstTurn * FirstSign);
	CandidateYawOffsets.Add(SecondTurn * -FirstSign);
	CandidateYawOffsets.Add(180.0f);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	for (const float YawOffset : CandidateYawOffsets)
	{
		FVector CandidateDirection = AwayDirection;
		if (FMath::IsNearlyEqual(YawOffset, 180.0f))
		{
			CandidateDirection = -GetDirectionAwayFromActor(ThreatActor);
		}
		CandidateDirection = CandidateDirection.RotateAngleAxis(YawOffset, FVector::UpVector);
		CandidateDirection.Z = 0.0f;
		if (!CandidateDirection.Normalize())
		{
			continue;
		}

		if (FVector::DotProduct(CandidateDirection, AwayDirection) < -KINDA_SMALL_NUMBER)
		{
			CandidateDirection = AwayDirection;
		}

		const FVector RawDestination = GetActorLocation() + (CandidateDirection * FleeDistance);
		FVector MoveDestination = RawDestination;
		if (NavSys)
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(RawDestination, NavLocation))
			{
				MoveDestination = NavLocation.Location;
			}
		}

		FVector ProjectedMoveDirection = MoveDestination - GetActorLocation();
		ProjectedMoveDirection.Z = 0.0f;
		if (!ProjectedMoveDirection.Normalize() || FVector::DotProduct(ProjectedMoveDirection, AwayDirection) < -KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (MoveToLocation(MoveDestination, AcceptanceRadius))
		{
			if (!FMath::IsNearlyZero(YawOffset) && bDrawDebug)
			{
				DebugMessage(FString::Printf(TEXT("Flee fallback used %.0f degree turn"), YawOffset), FColor::Orange, 1.5f);
			}
			return true;
		}
	}

	return false;
}

float APPCreatureBase::GetRandomFleeDuration() const
{
	const float MinDuration = FMath::Max(0.0f, FMath::Min(FleeDurationMin, FleeDurationMax));
	const float MaxDuration = FMath::Max(MinDuration, FMath::Max(FleeDurationMin, FleeDurationMax));
	return FMath::FRandRange(MinDuration, MaxDuration);
}

bool APPCreatureBase::GetRandomRoamLocation(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		DebugMessage(TEXT("No navigation system found for roaming"), FColor::Red);
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

bool APPCreatureBase::GetRandomIdleLocalWanderLocation(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		DebugMessage(TEXT("No navigation system found for idle wandering"), FColor::Red);
		return false;
	}

	FNavLocation NavLocation;
	if (NavSys->GetRandomReachablePointInRadius(GetActorLocation(), IdleLocalWanderRadius, NavLocation))
	{
		OutLocation = NavLocation.Location;
		return true;
	}

	return false;
}

float APPCreatureBase::GetRandomIdleStandDuration() const
{
	const float MinDuration = FMath::Max(0.0f, FMath::Min(IdleStandTimeMin, IdleStandTimeMax));
	const float MaxDuration = FMath::Max(MinDuration, FMath::Max(IdleStandTimeMin, IdleStandTimeMax));
	return FMath::FRandRange(MinDuration, MaxDuration);
}

bool APPCreatureBase::MoveToLocation(const FVector& TargetLocation, float AcceptanceRadius)
{
	if (!CanRequestMoveTo(TargetLocation))
	{
		DebugMessage(TEXT("Move request skipped: cooldown"), FColor::Silver, 0.75f);
		return true;
	}

	AAIController* AIController = EnsureCreatureAIController();
	if (!AIController)
	{
		bHasActiveMoveTarget = false;
		RegisterMoveRequestFailure(TargetLocation);
		DebugMessage(TEXT("Move request failed: no AI controller"), FColor::Red);
		return false;
	}

	CurrentMoveTarget = TargetLocation;
	LastMoveRequestTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastMoveRequestTime;
	const EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(TargetLocation, AcceptanceRadius);

	if (Result == EPathFollowingRequestResult::Failed)
	{
		bHasActiveMoveTarget = false;
		RegisterMoveRequestFailure(TargetLocation);
	}
	else
	{
		bHasActiveMoveTarget = Result == EPathFollowingRequestResult::RequestSuccessful;
		ResetMoveRequestFailures();
	}

	const bool bMoveRequestHandled = Result != EPathFollowingRequestResult::Failed;
	const TCHAR* ResultName = Result == EPathFollowingRequestResult::RequestSuccessful
		? TEXT("ok")
		: (Result == EPathFollowingRequestResult::AlreadyAtGoal ? TEXT("already at goal") : TEXT("failed"));
	DebugMessage(FString::Printf(TEXT("MoveTo %s result=%s"), *TargetLocation.ToCompactString(), ResultName), bMoveRequestHandled ? FColor::Cyan : FColor::Red);
	return bMoveRequestHandled;
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

	if (const AAIController* AIController = GetCreatureAIController())
	{
		if (AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			return true;
		}
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

void APPCreatureBase::RegisterMoveRequestFailure(const FVector& FailedTargetLocation)
{
	++ConsecutiveMoveFailures;

	DebugMessage(FString::Printf(
		TEXT("Move request failed %d / %d at %s"),
		ConsecutiveMoveFailures,
		MaxConsecutiveMoveFailures,
		*FailedTargetLocation.ToCompactString()),
		FColor::Red,
		1.5f);

	if (MaxConsecutiveMoveFailures > 0 && ConsecutiveMoveFailures >= MaxConsecutiveMoveFailures)
	{
		bHasActiveMoveTarget = false;
		CurrentMoveTarget = FVector::ZeroVector;
		LastMoveRequestTime = -1000.0f;
		ConsecutiveMoveFailures = 0;
		DebugMessage(TEXT("Move failed too often, clearing target for a fresh location"), FColor::Orange, 2.0f);
	}
}

void APPCreatureBase::ResetMoveRequestFailures()
{
	ConsecutiveMoveFailures = 0;
}

void APPCreatureBase::DebugMessage(const FString& Message, const FColor& Color, float Duration) const
{
	if (!bDrawDebug)
	{
		return;
	}

	const FString FullMessage = FString::Printf(TEXT("[%s] %s"), *GetName(), *Message);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *FullMessage);

	if (bDebugOnScreen && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color, FullMessage);
	}
}

bool APPCreatureBase::CanPrintDebugStatus() const
{
	if (!bDrawDebug)
	{
		return false;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (CurrentTime - LastDebugStatusTime < DebugStatusInterval)
	{
		return false;
	}

	LastDebugStatusTime = CurrentTime;
	return true;
}

void APPCreatureBase::DrawThreatDebug(AActor* ThreatActor, bool bInRange, bool bInCone)
{
	if (!bDrawDebug || !GetWorld())
	{
		return;
	}

	const FVector DebugOrigin = GetActorLocation() + FVector(0.0f, 0.0f, DebugVerticalOffset);
	const float DrawDuration = FMath::Max(DebugDrawDuration, AIUpdateInterval);
	const float LineThickness = FMath::Max(DebugLineThickness, 1.0f);
	const int32 SphereSegments = FMath::Max(DebugSphereSegments, 12);
	const FColor SightColor = bInRange ? FColor::Yellow : FColor::Silver;

	DrawDebugSphere(GetWorld(), DebugOrigin, SightThreatRadius, SphereSegments, SightColor, false, DrawDuration, 0, LineThickness);
	DrawDebugSphere(GetWorld(), DebugOrigin, SoundThreatRadius, SphereSegments, FColor::Blue, false, DrawDuration, 0, LineThickness);

	const float HalfSightAngle = SightThreatAngleDegrees * 0.5f;
	const FVector Forward = GetActorForwardVector();
	const FVector LeftSightEdge = Forward.RotateAngleAxis(-HalfSightAngle, FVector::UpVector);
	const FVector RightSightEdge = Forward.RotateAngleAxis(HalfSightAngle, FVector::UpVector);

	DrawDebugLine(GetWorld(), DebugOrigin, DebugOrigin + (LeftSightEdge * SightThreatRadius), SightColor, false, DrawDuration, 0, LineThickness);
	DrawDebugLine(GetWorld(), DebugOrigin, DebugOrigin + (RightSightEdge * SightThreatRadius), SightColor, false, DrawDuration, 0, LineThickness);
	DrawDebugDirectionalArrow(GetWorld(), DebugOrigin, DebugOrigin + (Forward * SightThreatRadius), 120.0f, FColor::Orange, false, DrawDuration, 0, LineThickness);

	if (ThreatActor)
	{
		const FVector ThreatDebugLocation = ThreatActor->GetActorLocation() + FVector(0.0f, 0.0f, DebugVerticalOffset);
		DrawDebugLine(
			GetWorld(),
			DebugOrigin,
			ThreatDebugLocation,
			(bInRange && bInCone) ? FColor::Red : FColor::Green,
			false,
			DrawDuration,
			0,
			LineThickness);
	}

	if (ThreatActor && CanPrintDebugStatus())
	{
		const float Distance = FVector::Dist(GetActorLocation(), ThreatActor->GetActorLocation());
		DebugMessage(FString::Printf(
			TEXT("Threat check: %s distance %.0f / %.0f, in range=%s, in cone=%s"),
			*GetNameSafe(ThreatActor),
			Distance,
			SightThreatRadius,
			bInRange ? TEXT("yes") : TEXT("no"),
			bInCone ? TEXT("yes") : TEXT("no")),
			(bInRange && bInCone) ? FColor::Red : FColor::Yellow,
			1.0f);
	}
}

FString APPCreatureBase::GetThreatDetectionName(EPPThreatDetectionType DetectionType) const
{
	switch (DetectionType)
	{
	case EPPThreatDetectionType::Sight:
		return TEXT("sight");
	case EPPThreatDetectionType::Sound:
		return TEXT("sound");
	default:
		return TEXT("none");
	}
}
