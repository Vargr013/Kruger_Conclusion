#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseGun.generated.h"

class ABaseProjectile;

UENUM(BlueprintType)
enum class EFireMode : uint8
{
    Projectile    UMETA(DisplayName = "Physical Bullet"),
    Raycast       UMETA(DisplayName = "Raycast / Spray")
};

UCLASS()
class KRUGER_CONCLUSION_API ABaseGun : public AActor
{
    GENERATED_BODY()

public:
    ABaseGun();

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* GunMesh;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* MuzzleLocation;

    // Weapon Settings
    UPROPERTY(EditDefaultsOnly, Category = "Weapon Setup")
    EFireMode FireMode = EFireMode::Projectile;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Setup")
    TSubclassOf<ABaseProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Setup|Raycast")
    float SprayRange = 500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Setup|Raycast")
    float DamagePerShot = 10.0f;

    void Shoot();

private:
    void FireProjectile();
    void FireRaycast();

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
    void OnRaycastHit(AActor* HitActor, FVector HitLocation);
};