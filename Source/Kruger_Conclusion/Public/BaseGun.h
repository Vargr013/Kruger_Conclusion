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

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Setup")
    EFireMode FireMode = EFireMode::Projectile;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Setup")
    TSubclassOf<ABaseProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Setup|Raycast")
    float SprayRange = 500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Setup|Raycast")
    float DamagePerShot = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Setup|HUD")
    FText ToolDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Setup|HUD", meta = (ClampMin = 0))
    int32 MaxAmmo = 10;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Setup", meta = (ClampMin = "0.05"))
    float ShotCooldownSeconds = 0.4f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Setup|HUD")
    bool bConsumesUses = true;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Reload() { CurrentAmmo = MaxAmmo; }

    void Shoot();

    UFUNCTION(BlueprintPure, Category = "Weapon Setup|HUD")
    int32 GetRemainingUses() const { return CurrentAmmo; }

    UFUNCTION(BlueprintPure, Category = "Weapon Setup|HUD")
    int32 GetMaxUses() const { return MaxAmmo; }

    UFUNCTION(BlueprintPure, Category = "Weapon Setup|HUD")
    FText GetToolDisplayName() const;

    UFUNCTION(BlueprintCallable, Category = "Weapon|Upgrades")
    void SetSpareMagazineCapacity(int32 Capacity);

    UFUNCTION(BlueprintCallable, Category = "Weapon|Upgrades")
    void RestoreSpareMagazines();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Upgrades")
    bool TryUseSpareMagazine();

    UFUNCTION(BlueprintPure, Category = "Weapon|Upgrades")
    int32 GetSpareMagazineCapacity() const { return SpareMagazineCapacity; }

    UFUNCTION(BlueprintPure, Category = "Weapon|Upgrades")
    int32 GetSpareMagazinesRemaining() const { return SpareMagazinesRemaining; }

    UFUNCTION(BlueprintCallable, Category = "Weapon|Upgrades")
    void ApplyRangeUpgrade();

private:
    bool FireProjectile();
    void FireRaycast();

    float NextShotTime = 0.0f;
    float BaseSprayRange = 500.0f;
    bool bRangeUpgradeApplied = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Weapon Setup|HUD")
    int32 CurrentAmmo = 0;

    UPROPERTY(VisibleInstanceOnly, Category = "Weapon|Upgrades")
    int32 SpareMagazineCapacity = 0;

    UPROPERTY(VisibleInstanceOnly, Category = "Weapon|Upgrades")
    int32 SpareMagazinesRemaining = 0;

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
    void OnRaycastHit(AActor* HitActor, FVector HitLocation);
};
