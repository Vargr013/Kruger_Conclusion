#include "Characters/PPAnimalCharacter.h"
#include "Data/PPAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnvironmentLevelSubsystem.h"
#include "Characters/PPPoacherCharacter.h"
#include "Actors/PPAnimalHerd.h"

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
	case EPPAnimalState::Investigating:
		return TEXT("Investigating");
	case EPPAnimalState::Chasing:
		return TEXT("Chasing");
	case EPPAnimalState::Searching:
		return TEXT("Searching");
	case EPPAnimalState::Attacking:
		return TEXT("Attacking");
	default:
		return TEXT("Unknown");
	}
}
}

APPAnimalCharacter::APPAnimalCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	StaticAnimalMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticAnimalMesh"));
	StaticAnimalMeshComponent->SetupAttachment(GetCapsuleComponent());
	StaticAnimalMeshComponent->SetCollisionProfileName(FName("NoCollision"));
	StaticAnimalMeshComponent->SetGenerateOverlapEvents(false);
	StaticAnimalMeshComponent->SetVisibility(false);

	AttackRange = 180.0f;
	AttackDamage = 25.0f;
	AttackCooldown = 1.25f;
	AttackAcceptanceRadius = 140.0f;
}

void APPAnimalCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyAnimalVisualMesh();
	if (!bUseStaticAnimalMeshVisual)
	{
		StartCreatureAnimation();
	}
	StartIdle();

	if (UWorld* World = GetWorld())
	{
		if (UEnvironmentLevelSubsystem* LevelSubsystem = World->GetSubsystem<UEnvironmentLevelSubsystem>())
		{
			LevelSubsystem->RegisterAnimal(this);
		}
	}
}

void APPAnimalCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyAnimalVisualMesh();
}

void APPAnimalCharacter::UpdateCreatureAI()
{
	const UEnvironmentLevelSubsystem* Rules = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr;
	if (Rules && Rules->IsPatrolSetupPending()) return;
	if (Rules && Rules->HasRoundEnded()) { StopMovement(); StopAIUpdates(); return; }
	if (bPoached)
	{
		return;
	}

	if (bIsPredator)
	{
		UpdatePredatorBehaviour();
		return;
	}

	AActor* ThreatActor = FindBestThreatActor();
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (ThreatActor && Herd.IsValid())
	{
		FPPThreatObservation Observation;
		if (ObserveTarget(ThreatActor, Observation)) Herd->ReportThreat(this, Observation.Location);
	}
	if (!ThreatActor && FleeFromHerdAlert(CurrentTime)) return;

	if (CurrentAnimalState == EPPAnimalState::Fleeing)
	{
		if (ThreatActor && CurrentThreatActor != ThreatActor)
		{
			StartFleeing(ThreatActor);
			return;
		}

		if (CurrentTime >= FleeEndTime)
		{
			StartIdle();
			return;
		}

		if (CurrentThreatActor)
		{
			TryMoveToFleeDestination(CurrentThreatActor, RoamAcceptanceRadius);
		}
		return;
	}

	if (ThreatActor)
	{
		StartFleeing(ThreatActor);
		return;
	}

	if (Herd.IsValid()) { UpdateHerdMovement(CurrentTime); return; }
	if (CurrentAnimalState == EPPAnimalState::Idle)
	{
		if (CurrentTime >= IdleEndTime)
		{
			StartRoaming();
		}
		else
		{
			UpdateIdleLocalWander(CurrentTime);
		}
		return;
	}

	if (CurrentAnimalState == EPPAnimalState::Roaming && IsCloseToCurrentMoveTarget(RoamAcceptanceRadius))
	{
		StartIdle();
	}
}

bool APPAnimalCharacter::IsValidThreatActor_Implementation(AActor* PotentialThreat) const
{
	if (!Super::IsValidThreatActor_Implementation(PotentialThreat))
	{
		return false;
	}

	if (PotentialThreat == FindPlayerActor())
	{
		return true;
	}

	if (Cast<APPPoacherCharacter>(PotentialThreat))
	{
		return true;
	}

	const APPAnimalCharacter* OtherAnimal = Cast<APPAnimalCharacter>(PotentialThreat);
	if (!OtherAnimal)
	{
		return false;
	}
	if (Herd.IsValid() && OtherAnimal->GetHerd() == Herd.Get()) return false;

	if (bIsPredator)
	{
		return false;
	}

	if (OtherAnimal->IsPredator())
	{
		return true;
	}

	// Elephants, zebras and rhinos share grazing areas without frightening each other.
	return false;
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
	if (Herd.IsValid()) { UpdateHerdMovement(GetWorld()->GetTimeSeconds()); return; }
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
	const float FleeDuration = GetRandomFleeDuration();
	FleeEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FleeDuration : 0.0f;
	DebugMessage(FString::Printf(TEXT("Fleeing from %s for %.1fs"), *GetNameSafe(ThreatActor), FleeDuration), FColor::Orange);

	if (!TryMoveToFleeDestination(ThreatActor, RoamAcceptanceRadius))
	{
		DebugMessage(TEXT("Flee move failed, returning to idle"), FColor::Red);
		StartIdle();
	}
}

void APPAnimalCharacter::StartIdle()
{
	ClearTargetMemory();
	CancelPredatorAttack();
	StopMovement();
	EnterRoamState();
	SetAnimalState(EPPAnimalState::Idle);

	const float IdleDuration = FMath::FRandRange(IdleTimeMin, IdleTimeMax);
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	IdleEndTime = CurrentTime + IdleDuration;
	NextIdleLocalWanderTime = CurrentTime + GetRandomIdleStandDuration();
	DebugMessage(FString::Printf(TEXT("Idling for %.1fs"), IdleDuration), FColor::Silver);
}

void APPAnimalCharacter::UpdateIdleLocalWander(float CurrentTime)
{
	if (Herd.IsValid()) return;
	if (CurrentTime < NextIdleLocalWanderTime)
	{
		return;
	}

	if (!bHasActiveMoveTarget || IsCloseToCurrentMoveTarget(IdleLocalWanderAcceptanceRadius))
	{
		FVector LocalWanderLocation;
		if (GetRandomIdleLocalWanderLocation(LocalWanderLocation))
		{
			DebugMessage(FString::Printf(TEXT("Idle local wander to %s"), *LocalWanderLocation.ToCompactString()), FColor::Silver);
			MoveToLocation(LocalWanderLocation, IdleLocalWanderAcceptanceRadius);
		}
	}

	NextIdleLocalWanderTime = CurrentTime + GetRandomIdleStandDuration();
}

void APPAnimalCharacter::ApplyAnimalVisualMesh()
{
	if (!bUseStaticAnimalMeshVisual || !StaticAnimalMeshComponent)
	{
		if (StaticAnimalMeshComponent)
		{
			StaticAnimalMeshComponent->SetVisibility(false, true);
			StaticAnimalMeshComponent->SetHiddenInGame(true, true);
		}
		if (USkeletalMeshComponent* CharacterMesh = GetMesh())
		{
			CharacterMesh->SetVisibility(true, true);
			CharacterMesh->SetHiddenInGame(false, true);
		}
		if (!bUseStaticAnimalMeshVisual)
		{
			PrepareCreatureAnimationVisual();
		}
		return;
	}

	UStaticMesh* LoadedMesh = StaticAnimalMeshComponent->GetStaticMesh();
	if (!StaticAnimalMeshOverride.IsNull())
	{
		LoadedMesh = StaticAnimalMeshOverride.LoadSynchronous();
	}

	if (!LoadedMesh)
	{
		if (USkeletalMeshComponent* CharacterMesh = GetMesh())
		{
			CharacterMesh->SetVisibility(true, true);
			CharacterMesh->SetHiddenInGame(false, true);
		}
		return;
	}

	StaticAnimalMeshComponent->SetStaticMesh(LoadedMesh);
	StaticAnimalMeshComponent->SetVisibility(true, true);
	StaticAnimalMeshComponent->SetHiddenInGame(false, true);

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetVisibility(false, true);
		CharacterMesh->SetHiddenInGame(true, true);
	}
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

void APPAnimalCharacter::HandleHealthDepleted()
{
	if (bPoached)
	{
		return;
	}

	bPoached = true;
	if (Herd.IsValid()) Herd->RemoveMember(this);
	StopMovement();
	StopAIUpdates();
	SetActorEnableCollision(false);

	if (UWorld* World = GetWorld())
	{
		if (UEnvironmentLevelSubsystem* LevelSubsystem = World->GetSubsystem<UEnvironmentLevelSubsystem>())
		{
			// I kept predator losses separate from confirmed poacher kills.
			LevelSubsystem->ReportAnimalLost(this, Cast<APPPoacherCharacter>(LastDamageCauser) != nullptr);
		}

		if (bRemoveAfterPoached)
		{
			const float SafeDelay = FMath::Max(0.0f, PoachedRemovalDelay);
			if (SafeDelay <= 0.0f)
			{
				FinalizePoachedRemoval();
			}
			else
			{
				GetWorldTimerManager().SetTimer(PoachedRemovalTimerHandle, this, &APPAnimalCharacter::FinalizePoachedRemoval, SafeDelay, false);
			}
		}
	}

	DebugMessage(TEXT("Animal poached"), FColor::Red, 3.0f);
}

void APPAnimalCharacter::FinalizePoachedRemoval()
{
	if (!bPoached)
	{
		return;
	}

	SetActorHiddenInGame(true);
	Destroy();
}

bool APPAnimalCharacter::CanAttackTarget(AActor* PotentialTarget) const
{
	if (!bIsPredator || !Super::CanAttackTarget(PotentialTarget))
	{
		return false;
	}

	if (PotentialTarget == FindPlayerActor())
	{
		return true;
	}

	if (Cast<APPPoacherCharacter>(PotentialTarget))
	{
		return true;
	}

	const APPAnimalCharacter* OtherAnimal = Cast<APPAnimalCharacter>(PotentialTarget);
	return OtherAnimal && !OtherAnimal->IsPredator();
}
