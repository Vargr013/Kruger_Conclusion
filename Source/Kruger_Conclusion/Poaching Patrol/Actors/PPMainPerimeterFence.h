#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPMainPerimeterFence.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;

/** Editor-authored, instanced visual fence. Existing perimeter walls own collision. */
UCLASS()
class KRUGER_CONCLUSION_API APPMainPerimeterFence : public AActor
{
	GENERATED_BODY()

public:
	APPMainPerimeterFence();

	UFUNCTION(BlueprintCallable, Category="Perimeter Fence")
	void AddSection(const FTransform& WorldTransform);

	UFUNCTION(BlueprintCallable, Category="Perimeter Fence")
	void ClearSections();

	UFUNCTION(BlueprintPure, Category="Perimeter Fence")
	int32 GetSectionCount() const;

private:
	UPROPERTY(VisibleAnywhere, Category="Perimeter Fence")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Perimeter Fence")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Bars;

	UPROPERTY(VisibleAnywhere, Category="Perimeter Fence")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Mesh;
};
