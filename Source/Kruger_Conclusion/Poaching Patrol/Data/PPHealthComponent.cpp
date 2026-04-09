#include "Data/PPHealthComponent.h"

UPPHealthComponent::UPPHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPPHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

void UPPHealthComponent::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f || IsDead()) return;

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (IsDead())
	{
		OnDeath.Broadcast();
	}
}

void UPPHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.0f || IsDead()) return;

	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}
