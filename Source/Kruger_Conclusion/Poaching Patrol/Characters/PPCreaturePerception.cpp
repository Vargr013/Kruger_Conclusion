#include "Characters/PPCreatureBase.h"
#include "Data/PPNoiseEmitterComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

namespace
{
FCollisionQueryParams SightQuery(const AActor* Observer, AActor* Target)
{
	FCollisionQueryParams Query(SCENE_QUERY_STAT(PatrolSight), false, Observer);
	Query.AddIgnoredActor(Target);
	TArray<AActor*> Attached;
	Observer->GetAttachedActors(Attached, true, true);
	Query.AddIgnoredActors(Attached);
	Target->GetAttachedActors(Attached, true, true);
	Query.AddIgnoredActors(Attached);
	return Query;
}
}

bool APPCreatureBase::CanSeeTarget(AActor* Target) const
{
	if (!IsValid(Target) || !GetWorld() || !bUseSightThreats || Target->IsHidden() || IsActorHealthDepleted(Target)) return false;
	const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	if (DistanceSquared > FMath::Square(SightThreatRadius) || !IsThreatInForwardCone(Target)) return false;
	FVector Eye; FRotator Rotation;
	GetActorEyesViewPoint(Eye, Rotation);
	FVector TargetEye = Target->GetActorLocation() + FVector(0, 0, 50);
	FVector Torso = Target->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		Character->GetActorEyesViewPoint(TargetEye, Rotation);
		Torso.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.3f;
	}
	const FCollisionQueryParams Query = SightQuery(this, Target);
	for (const FVector& Sample : {TargetEye, Torso})
	{
		if (GetWorld()->LineTraceTestByChannel(Eye, Sample, ECC_Visibility, Query)) continue;
		if (DistanceSquared <= FMath::Square(200.0f)
			|| !GetWorld()->LineTraceTestByChannel(Eye, Sample, ECC_GameTraceChannel2, Query)) return true;
	}
	return false;
}

bool APPCreatureBase::ObserveTarget(AActor* Target, FPPThreatObservation& Observation) const
{
	Observation = FPPThreatObservation();
	if (!IsValid(Target) || Target == this || !GetWorld() || Target->IsHidden() || IsActorHealthDepleted(Target)) return false;
	Observation.bSeen = CanSeeTarget(Target);
	if (bUseSoundThreats)
	{
		if (const UPPNoiseEmitterComponent* Noise = Target->FindComponentByClass<UPPNoiseEmitterComponent>())
		{
			TArray<FPPNoiseStimulus> Stimuli;
			Noise->GetActiveStimuli(Stimuli);
			const FCollisionQueryParams Query = SightQuery(this, Target);
			for (const FPPNoiseStimulus& Stimulus : Stimuli)
			{
				float Range = FMath::Min(SoundThreatRadius, Stimulus.Radius);
				const float Distance = FVector::DistSquared(GetActorLocation(), Stimulus.Location);
				if (Distance > FMath::Square(Range)) continue;
				if (GetWorld()->LineTraceTestByChannel(GetActorLocation(), Stimulus.Location, ECC_Visibility, Query)) Range *= 0.5f;
				if (Distance <= FMath::Square(Range) && Stimulus.Time >= Observation.Time)
				{
					Observation.bHeard = true;
					Observation.Location = Stimulus.Location;
					Observation.Time = Stimulus.Time;
				}
			}
		}
	}
	if (Observation.bSeen)
	{
		Observation.Location = Target->GetActorLocation();
		Observation.Time = GetWorld()->GetTimeSeconds();
	}
	return Observation.bSeen || Observation.bHeard;
}

void APPCreatureBase::RememberObservation(AActor* Target, const FPPThreatObservation& Observation)
{
	if (!Target || (!Observation.bSeen && !Observation.bHeard)) return;
	if (Observation.bSeen || RememberedTarget != Target || Observation.Time > LastObservation.Time)
	{
		SearchPointIndex = 0;
		NextSearchPointTime = 0.0f;
		LastObservation = Observation;
		RememberedTarget = Target;
	}
}

void APPCreatureBase::ClearTargetMemory()
{
	RememberedTarget.Reset();
	LastObservation = FPPThreatObservation();
	SearchPointIndex = 0;
	NextSearchPointTime = 0.0f;
}

bool APPCreatureBase::IsSearchingLastLocation(float MemorySeconds) const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() - LastObservation.Time > MemorySeconds;
}

bool APPCreatureBase::FollowRememberedTarget(float Leash, float MemorySeconds)
{
	if (!GetWorld() || !RememberedTarget.IsValid() || IsActorHealthDepleted(RememberedTarget.Get())) return false;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastObservation.Time > MemorySeconds + 4.0f
		|| FVector::DistSquared(HomeLocation, LastObservation.Location) > FMath::Square(Leash)) return false;
	FVector Facing = LastObservation.Location - GetActorLocation();
	Facing.Z = 0;
	if (Facing.Normalize()) SetActorRotation(Facing.Rotation());
	if (Now - LastObservation.Time <= MemorySeconds) return MoveToLocation(LastObservation.Location, 100.0f);
	if (SearchPointIndex < 2 && Now >= NextSearchPointTime)
	{
		UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
		FNavLocation Point;
		if (!Navigation || !Navigation->GetRandomReachablePointInRadius(LastObservation.Location, 300.0f, Point)) return false;
		++SearchPointIndex;
		NextSearchPointTime = Now + 2.0f;
		return MoveToLocation(Point.Location, 100.0f);
	}
	return true;
}
