#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/PPAnimalSpawnProfile.h"
#include "PPSpawnDirector.generated.h"

class UBoxComponent;
class APPPoacherCharacter;
class APPAnimalCharacter;
class APPCreatureBase;
class APPAnimalHerd;
class UWorldPartitionStreamingSourceComponent;

USTRUCT(BlueprintType)
struct FPPAnimalSpawnPlacement
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TObjectPtr<APPAnimalCharacter> Animal;
	UPROPERTY(BlueprintReadOnly) FTransform Transform;
	UPROPERTY(BlueprintReadOnly) int32 GroupId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 ProfileIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) bool bUsedFallback = false;
};

USTRUCT(BlueprintType)
struct FPPAnimalGroupPlan
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FVector Center = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) int32 ProfileIndex = INDEX_NONE;
};

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
	/** Keeps collision and navigation around the relocated roster available across the playable area. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawning|Area") TObjectPtr<UWorldPartitionStreamingSourceComponent> PopulationStreamingSource;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Animals") bool bRandomiseAnimals = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Animals") TArray<FPPAnimalSpawnProfile> AnimalProfiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning|Animals", meta=(ClampMin="1", ClampMax="8")) int32 LayoutAttempts = 4;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") TArray<FPPAnimalSpawnPlacement> AnimalPlacements;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") TArray<FPPAnimalGroupPlan> AnimalGroups;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") TArray<TObjectPtr<APPAnimalHerd>> Herds;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") int32 ActiveSeed = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") TArray<FPPSpawnPlacement> Placements;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") TMap<FName, int32> Rejections;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Spawning|Debug") FString LastError;

	bool PreparePatrol(AActor* Player);
	bool HasFailed() const { return bPreparationAttempted && !bPrepared; }
	bool IsWaitingForWorld() const { return bWaitingForWorld; }
	/** Validation preview does not move actors, start AI, or register new actors. */
	UFUNCTION(BlueprintCallable, Category="Spawning|Debug") bool PreviewLayout(int32 Seed);
	bool BuildLayout(int32 Seed, const FVector& PlayerLocation, TArray<FPPSpawnPlacement>& OutPlacements);
	bool BuildAuthoredLayout(int32 Seed, const FVector& PlayerLocation, TArray<FPPSpawnPlacement>& OutPlacements);
	bool IsInsidePlayableArea(const FVector& Location, float Inset = 0.0f) const;
	/** Shared terrain and clearance validation for placement and herd destinations. */
	bool ProjectCreatureLocation(APPCreatureBase* Creature, const FVector& Raw, FVector& Out, FName& Reason,
		const FQuat* Facing = nullptr) const;
	bool HasCreatureRoute(const APPCreatureBase* Creature, const FVector& Start, const FVector& End, float MaxLength) const;
	static float GetBodyRadius(const APPCreatureBase* Creature);
	static bool PartitionPopulation(int32 Total, int32 Minimum, int32 Maximum, FRandomStream& Random, TArray<int32>& Sizes);

private:
	bool bPreparationAttempted = false;
	bool bPrepared = false;
	bool bWaitingForWorld = false;
	bool bStreamingRequested = false;
	bool BuildAnimalLayout(int32 Seed, const FVector& PlayerLocation, bool bAuthoredFallback = false);
	bool BuildPoacherLayout(int32 Seed, const FVector& PlayerLocation, TArray<FPPSpawnPlacement>& OutPlacements);
	int32 FindAnimalProfile(const APPAnimalCharacter* Animal) const;
	bool ValidateAnimalPosition(APPAnimalCharacter* Animal, const FVector& Raw, const FVector& PlayerLocation,
		int32 ProfileIndex, int32 GroupId, FVector& OutLocation, FName& Reason) const;
	bool ValidateCandidate(APPPoacherCharacter* Poacher, const FVector& RawLocation, const FVector& PlayerLocation,
		const TArray<APPPoacherCharacter*>& Roster, const TArray<FPPSpawnPlacement>& Planned,
		FVector& OutLocation, FName& Reason) const;
	void RecordRejection(const FVector& Location, FName Reason);
};
