#include "Characters/PPAnimalCharacter.h"
#include "Actors/PPAnimalHerd.h"

APPAnimalHerd* APPAnimalCharacter::GetHerd() const { return Herd.Get(); }

void APPAnimalCharacter::SetHerd(APPAnimalHerd* NewHerd)
{
	Herd = NewHerd;
	HerdAlertUntil = -1000;
	NextHerdMoveTime = 0;
}

void APPAnimalCharacter::ReceiveHerdAlert(const FVector& LastKnownLocation, float UntilTime)
{
	if (bPoached || bIsPredator || !Herd.IsValid()) return;
	HerdThreatLocation = LastKnownLocation;
	HerdAlertUntil = UntilTime;
}

bool APPAnimalCharacter::FleeFromHerdAlert(float Now)
{
	if (Now >= HerdAlertUntil || !Herd.IsValid()) return false;
	FVector Away = GetActorLocation() - HerdThreatLocation;
	Away.Z = 0;
	if (!Away.Normalize()) Away = GetActorForwardVector();
	CurrentThreatActor = nullptr;
	SetCreatureMoveSpeed(FleeSpeed);
	SetAnimalState(EPPAnimalState::Fleeing);
	FleeEndTime = FMath::Max(FleeEndTime, Now + 1.0f);
	if (Now < NextHerdMoveTime) return true;
	NextHerdMoveTime = Now + FMath::Max(0.75f, Herd->Profile.UpdateSeconds);
	for (float Angle : {0.0f, 45.0f, -45.0f, 90.0f, -90.0f})
	{
		FVector Goal;
		if (Herd->FindSafeGoal(this, GetActorLocation() + Away.RotateAngleAxis(Angle, FVector::UpVector) * FleeDistance, Goal)
			&& MoveToLocation(Goal, RoamAcceptanceRadius)) return true;
	}
	StopMovement();
	return true;
}

void APPAnimalCharacter::UpdateHerdMovement(float Now)
{
	if (!Herd.IsValid()) return;
	if (Now < NextHerdMoveTime) return;
	NextHerdMoveTime = Now + FMath::Max(0.75f, Herd->Profile.UpdateSeconds);
	FVector Goal;
	bool bCrowded = false;
	if (Herd->GetSeparationGoal(this, Goal, bCrowded))
	{
		EnterRoamState(); SetAnimalState(EPPAnimalState::Roaming);
		if (!MoveToLocation(Goal, RoamAcceptanceRadius)) StartIdle();
		return;
	}
	if (bCrowded) { StopMovement(); return; }
	if (CurrentAnimalState == EPPAnimalState::Idle && Now < IdleEndTime && !Herd->ShouldRegroup(this)) return;
	if (!Herd->GetRoamGoal(this, Goal))
	{
		if (CurrentAnimalState != EPPAnimalState::Idle) StartIdle();
		return;
	}
	if (FVector::DistSquared2D(GetActorLocation(), Goal) <= FMath::Square(RoamAcceptanceRadius + 40))
	{
		if (CurrentAnimalState != EPPAnimalState::Idle) StartIdle();
		return;
	}
	if (CurrentAnimalState == EPPAnimalState::Roaming && HasActiveMoveTarget()
		&& FVector::DistSquared(Goal, GetCurrentMoveTarget()) < FMath::Square(100) && !IsCloseToCurrentMoveTarget(RoamAcceptanceRadius)) return;
	EnterRoamState();
	SetAnimalState(EPPAnimalState::Roaming);
	if (!MoveToLocation(Goal, RoamAcceptanceRadius)) StartIdle();
}
