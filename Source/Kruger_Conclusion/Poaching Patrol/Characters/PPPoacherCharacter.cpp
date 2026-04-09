#include "Characters/PPPoacherCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

APPPoacherCharacter::APPPoacherCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APPPoacherCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APPPoacherCharacter::SetPoacherState(EPoacherState NewState)
{
	if (bCaptured) return;
	CurrentState = NewState;
}

void APPPoacherCharacter::SetTargetActor(AActor* NewTarget)
{
	CurrentTargetActor = NewTarget;
}

void APPPoacherCharacter::SetThreatActor(AActor* NewThreat)
{
	CurrentThreatActor = NewThreat;
}

void APPPoacherCharacter::CapturePoacher()
{
	bCaptured = true;
	CurrentState = EPoacherState::Captured;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
}
