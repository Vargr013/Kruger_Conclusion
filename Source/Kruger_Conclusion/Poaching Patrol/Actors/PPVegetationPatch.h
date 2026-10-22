#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPVegetationPatch.generated.h"

class UBoxComponent;
class UHierarchicalInstancedStaticMeshComponent;

/** Saved clumps pair visible vegetation with smaller, passable sight cores. */
UCLASS()
class KRUGER_CONCLUSION_API APPVegetationPatch : public AActor
{
	GENERATED_BODY()
public:
	APPVegetationPatch();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vegetation") FName PatchId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vegetation") int32 Seed = 20261020;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vegetation") TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Bushes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vegetation") TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GrassOne;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vegetation") TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GrassTwo;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vegetation") TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GrassThree;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vegetation") TArray<TObjectPtr<UBoxComponent>> SightCores;
	UFUNCTION(BlueprintCallable, Category="Vegetation") void AddBush(const FTransform& WorldTransform);
	UFUNCTION(BlueprintCallable, Category="Vegetation") void AddGrass(int32 Variant, const FTransform& WorldTransform);
	UFUNCTION(BlueprintCallable, Category="Vegetation") void AddSightCore(FVector WorldCentre, FVector Extent, float Yaw = 0.0f);
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Vegetation") void ClearPatch();
	UFUNCTION(BlueprintPure, Category="Vegetation") int32 GetInstanceCount() const;
	UFUNCTION(BlueprintPure, Category="Vegetation") bool HasProtectedCoverSettings() const;
};
