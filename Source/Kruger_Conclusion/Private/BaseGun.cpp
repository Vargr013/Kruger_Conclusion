#include "BaseGun.h"
#include "BaseProjectile.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ABaseGun::ABaseGun()
{
    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
    GunMesh->SetupAttachment(SceneRoot);

    MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
    MuzzleLocation->SetupAttachment(SceneRoot);
}

void ABaseGun::Shoot()
{
    if (FireMode == EFireMode::Projectile)
    {
        FireProjectile();
    }
    else if (FireMode == EFireMode::Raycast)
    {
        FireRaycast();
    }
}

void ABaseGun::FireProjectile()
{
    if (!ProjectileClass) return;

    FVector Loc = MuzzleLocation->GetComponentLocation();
    FRotator Rot = MuzzleLocation->GetComponentRotation();

    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.Instigator = GetInstigator();

    GetWorld()->SpawnActor<ABaseProjectile>(ProjectileClass, Loc, Rot, Params);
}

void ABaseGun::FireRaycast()
{
    FVector Start = MuzzleLocation->GetComponentLocation();

    // Get the forward direction the muzzle is facing.
    FVector ForwardVector = MuzzleLocation->GetForwardVector();
    FVector End = Start + (ForwardVector * SprayRange);

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(GetOwner()); // Doesn't spray self.

    // Perform the Raycast (Line trace) looking for the Visibility channel.
    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

    // This draws the spray line in-game. Green if it hit something, orange if it didn't.
    FColor LineColor = bHit ? FColor::Green : FColor::Orange;
    DrawDebugLine(GetWorld(), Start, End, LineColor, false, 0.5f, 0, 2.0f);

    if (bHit && HitResult.GetActor())
    {
        // Deal damage directly to whatever the pepper spray hit.
        UGameplayStatics::ApplyDamage(
            HitResult.GetActor(),
            DamagePerShot,
            GetInstigatorController(),
            this,
            UDamageType::StaticClass()
        );

        // Trigger the Blueprint Event and pass along who we hit. 
        OnRaycastHit(HitResult.GetActor(), HitResult.ImpactPoint);

        // Debug message showing who you hit.
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("Sprayed: %s"), *HitResult.GetActor()->GetName()));
    }
}



