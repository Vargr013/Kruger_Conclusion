#include "BaseGun.h"
#include "BaseProjectile.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
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

    CurrentUses = MaxUses;
}

void ABaseGun::Shoot()
{
    if (bConsumesUses && CurrentUses <= 0)
    {
        return;
    }

    if (FireMode == EFireMode::Projectile)
    {
        FireProjectile();
    }
    else if (FireMode == EFireMode::Raycast)
    {
        FireRaycast();
    }

    if (bConsumesUses)
    {
        CurrentUses = FMath::Max(0, CurrentUses - 1);
    }
}

void ABaseGun::BeginPlay()
{
    Super::BeginPlay();

    CurrentUses = MaxUses;
}

FText ABaseGun::GetToolDisplayName() const
{
    if (!ToolDisplayName.IsEmpty())
    {
        return ToolDisplayName;
    }

    return FireMode == EFireMode::Raycast
        ? FText::FromString(TEXT("Pepper Spray"))
        : FText::FromString(TEXT("Darts"));
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
        if (APPPoacherCharacter* Poacher = Cast<APPPoacherCharacter>(HitResult.GetActor()))
        {
            Poacher->ApplyPepperSpraySlow();
        }
        else if (!Cast<APPAnimalCharacter>(HitResult.GetActor()))
        {
            UGameplayStatics::ApplyDamage(
                HitResult.GetActor(),
                DamagePerShot,
                GetInstigatorController(),
                this,
                UDamageType::StaticClass()
            );
        }

        // Trigger the Blueprint Event and pass along who we hit. 
        OnRaycastHit(HitResult.GetActor(), HitResult.ImpactPoint);

        // Debug message showing who you hit.
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("Sprayed: %s"), *HitResult.GetActor()->GetName()));
    }
}



