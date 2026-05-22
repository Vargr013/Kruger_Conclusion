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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Setup|HUD")
    FText ToolDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Setup|HUD", meta = (ClampMin = 0))
    int32 MaxUses = 8;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Setup|HUD")
    bool bConsumesUses = true;

    void Shoot();

    UFUNCTION(BlueprintPure, Category = "Weapon Setup|HUD")
    int32 GetRemainingUses() const { return CurrentUses; }

    UFUNCTION(BlueprintPure, Category = "Weapon Setup|HUD")
    int32 GetMaxUses() const { return MaxUses; }

    UFUNCTION(BlueprintPure, Category = "Weapon Setup|HUD")
    FText GetToolDisplayName() const;

private:
    void FireProjectile();
    void FireRaycast();

    UPROPERTY(VisibleInstanceOnly, Category = "Weapon Setup|HUD")
    int32 CurrentUses = 0;

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
    void OnRaycastHit(AActor* HitActor, FVector HitLocation);
};
