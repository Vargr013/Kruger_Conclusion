#include "Characters/PPAnimalCharacter.h"
#include "Data/PPHealthComponent.h"

APPAnimalCharacter::APPAnimalCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	HealthComponent = CreateDefaultSubobject<UPPHealthComponent>(TEXT("HealthComponent"));
}

void APPAnimalCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APPAnimalCharacter::SetAnimalState(EAnimalState NewState)
{
	CurrentState = NewState;
}

void APPAnimalCharacter::SetThreatActor(AActor* NewThreat)
{
	CurrentThreatActor = NewThreat;
}

bool APPAnimalCharacter::IsHealthLow() const
{
	if (!HealthComponent)
	{
		return false;
	}

	return HealthComponent->GetCurrentHealth() <= (HealthComponent->GetMaxHealth() * 0.3f);
}

FVector APPAnimalCharacter::GetFleeLocation(float Distance) const
{
	if (!CurrentThreatActor)
	{
		return GetActorLocation();
	}

	FVector AwayDirection = GetActorLocation() - CurrentThreatActor->GetActorLocation();
	AwayDirection.Z = 0.0f;
	AwayDirection.Normalize();

	return GetActorLocation() + (AwayDirection * Distance);
}
