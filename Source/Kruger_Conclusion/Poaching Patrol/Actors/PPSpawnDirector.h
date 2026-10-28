#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPSpawnDirector.generated.h"

class UBoxComponent;
class APPPoacherCharacter;

USTRUCT(BlueprintType)
struct FPPSpawnPlacement
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TObjectPtr<APPPoacherCharacter> Poacher;
	UPROPERTY(BlueprintReadOnly) FTransform Transform;
	UPROPERTY(BlueprintReadOnly) bool bUsedFallback = false;
};

/** Relocates the authored normal-patrol roster once, before the patrol clock starts. */
UCLASS()
class KRUGER_CONCLUSION_API APPSpawnDirector : public AActor
{
	GENERATED_BODY()
public:
	APPSpawnDirector();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawning|Area") TObjectPtr<UBoxComponent> SpawnBounds;
	/** World XY coordinates following the playable perimeter, in clockwise or anticlockwise order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Area") TArray<FVector2D> PlayablePolygon;
	/** Bounds of these actors are excluded, including water or other designer-defined unsafe areas. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Spawning|Area") TArray<TObjectPtr<AActor>> ExclusionAreas;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Distances", meta=(ClampMin="0")) float BoundaryInset = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Distances", meta=(ClampMin="0")) float PlayerSafeDistance = 3500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Distances", meta=(ClampMin="0")) float CampSafeDistance = 3000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Distances", meta=(ClampMin="0")) float PoacherSpacing = 3000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Distances", meta=(ClampMin="0")) float AnimalSafeDistance = 2200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Navigation", meta=(ClampMin="100")) float MaxEscortPathLength = 45000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Navigation", meta=(ClampMin="100")) float RoamProbeDistance = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Generation", meta=(ClampMin="1", ClampMax="500")) int32 AttemptsPerRegion = 100;
	/** Zero generates a new seed per patrol. A nonzero value reproduces the starting layout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Generation") int32 FixedSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Debug") bool bDrawSpawnDebug = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") int32 ActiveSeed = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") TArray<FPPSpawnPlacement> Placements;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") TMap<FName, int32> Rejections;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") FString LastError;

	bool PreparePatrol(AActor* Player);
	bool HasFailed() const { return bPreparationAttempted && !bPrepared; }
	/** Validation preview does not move actors, start AI, or register new actors. */
	UFUNCTION(BlueprintCallable, Category="Spawning|Debug") bool PreviewLayout(int32 Seed);
	bool BuildLayout(int32 Seed, const FVector& PlayerLocation, TArray<FPPSpawnPlacement>& OutPlacements);
	bool IsInsidePlayableArea(const FVector& Location, float Inset = 0.0f) const;

private:
	bool bPreparationAttempted = false;
	bool bPrepared = false;
	bool ValidateCandidate(APPPoacherCharacter* Poacher, const FVector& RawLocation, const FVector& PlayerLocation,
		const TArray<APPPoacherCharacter*>& Roster, const TArray<FPPSpawnPlacement>& Planned,
		FVector& OutLocation, FName& Reason) const;
	void RecordRejection(const FVector& Location, FName Reason);
};
