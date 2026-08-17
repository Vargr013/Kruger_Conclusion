#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PPMinimapDefinition.generated.h"

class UTexture2D;
class UMaterialInterface;

UCLASS(BlueprintType)
class KRUGER_CONCLUSION_API UPPMinimapDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol|Minimap")
	TObjectPtr<UTexture2D> BackgroundTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol|Minimap")
	TObjectPtr<UMaterialInterface> BackgroundMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol|Minimap")
	FVector2D WorldBoundsMin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol|Minimap")
	FVector2D WorldBoundsMax = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol|Minimap")
	float TextureRotationDegrees = 0.0f;

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Minimap")
	bool IsValidDefinition() const
	{
		return (BackgroundTexture != nullptr || BackgroundMaterial != nullptr)
			&& WorldBoundsMax.X > WorldBoundsMin.X
			&& WorldBoundsMax.Y > WorldBoundsMin.Y;
	}

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Minimap")
	FVector2D WorldToTextureUV(const FVector2D& WorldLocation) const
	{
		const FVector2D BoundsSize = WorldBoundsMax - WorldBoundsMin;
		return BoundsSize.X > KINDA_SMALL_NUMBER && BoundsSize.Y > KINDA_SMALL_NUMBER
			? (WorldLocation - WorldBoundsMin) / BoundsSize
			: FVector2D(-1.0f, -1.0f);
	}

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Minimap")
	bool IsWorldLocationMapped(const FVector2D& WorldLocation) const
	{
		const FVector2D UV = WorldToTextureUV(WorldLocation);
		return UV.X >= 0.0f && UV.X <= 1.0f && UV.Y >= 0.0f && UV.Y <= 1.0f;
	}
};
