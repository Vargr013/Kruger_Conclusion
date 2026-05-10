#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseGun.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API ABaseGun : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseGun();
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* GunMesh;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* MuzzleLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class ABaseProjectile> ProjectileClass;

	void Shoot();
};
