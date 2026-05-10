#include "BaseGun.h"
#include "BaseProjectile.h"

// Sets default values
ABaseGun::ABaseGun()
{
	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	RootComponent = GunMesh;

	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	MuzzleLocation->SetupAttachment(RootComponent);
}

void ABaseGun::Shoot()
{
	if (ProjectileClass)
	{
		FVector MuzzleLocationWorld = MuzzleLocation->GetComponentLocation();
		FRotator MuzzleRotationWorld = MuzzleLocation->GetComponentRotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = GetInstigator();
		GetWorld()->SpawnActor<ABaseProjectile>(ProjectileClass, MuzzleLocationWorld, MuzzleRotationWorld, SpawnParams);
	}
}



