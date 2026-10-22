#include "Data/PPNoiseEmitterComponent.h"
#include "ABaseCharacter.h"
#include "Data/PPHealthComponent.h"
#include "EnvironmentLevelSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

UPPNoiseEmitterComponent::UPPNoiseEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

float UPPNoiseEmitterComponent::MovementRadius(bool bCrouched, bool bFast, float Speed)
{
	return Speed <= 30.0f ? 0.0f : bCrouched ? 200.0f : bFast ? 1200.0f : 600.0f;
}

void UPPNoiseEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UPPHealthComponent* Health = GetOwner()->FindComponentByClass<UPPHealthComponent>();
	const UEnvironmentLevelSubsystem* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>();
	const APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Character || Character->IsHidden() || (Health && Health->IsDead()) || (Rules && Rules->HasRoundEnded())
		|| (PC && (PC->IsMoveInputIgnored() || PC->bShowMouseCursor)))
	{
		ClearNoise();
		return;
	}
	const float Speed = Character->GetVelocity().Size2D();
	const ABaseCharacter* Ranger = Cast<ABaseCharacter>(Character);
	const bool bFast = Ranger ? Ranger->IsSprinting() : Speed >= FastMovementSpeedThreshold;
	const float Radius = Character->GetCharacterMovement()->IsMovingOnGround()
		? MovementRadius(Character->bIsCrouched, bFast, Speed) : 0.0f;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Radius > 0.0f && Now >= NextStepTime)
	{
		MovementNoise.Location = Character->GetActorLocation();
		MovementNoise.Radius = Radius;
		MovementNoise.Time = Now;
		NextStepTime = Now + (Character->bIsCrouched ? 0.65f : bFast ? 0.35f : 0.5f);
	}
}

void UPPNoiseEmitterComponent::EmitNoise(float Radius)
{
	if (!GetWorld() || GetWorld()->IsPaused() || GetOwner()->IsHidden()) return;
	ActionNoise.Location = GetOwner()->GetActorLocation();
	ActionNoise.Radius = FMath::Max(0.0f, Radius);
	ActionNoise.Time = GetWorld()->GetTimeSeconds();
	ActionNoise.Lifetime = 1.0f;
}

void UPPNoiseEmitterComponent::ClearNoise()
{
	MovementNoise = FPPNoiseStimulus();
	ActionNoise = FPPNoiseStimulus();
	NextStepTime = 0.0f;
}

void UPPNoiseEmitterComponent::GetActiveStimuli(TArray<FPPNoiseStimulus>& OutStimuli) const
{
	if (!GetWorld()) return;
	const float Now = GetWorld()->GetTimeSeconds();
	if (MovementNoise.IsActive(Now)) OutStimuli.Add(MovementNoise);
	if (ActionNoise.IsActive(Now)) OutStimuli.Add(ActionNoise);
}
