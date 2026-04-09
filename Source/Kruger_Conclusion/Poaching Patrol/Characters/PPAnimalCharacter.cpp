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

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &APPAnimalCharacter::HandleDeath);
	}
}

void APPAnimalCharacter::SetAnimalState(EAnimalState NewState)
{
	CurrentState = NewState;
}

void APPAnimalCharacter::HandleDeath()
{
	Destroy();
}
