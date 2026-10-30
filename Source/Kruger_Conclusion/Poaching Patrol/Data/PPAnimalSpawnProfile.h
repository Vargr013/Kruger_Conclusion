#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PPAnimalSpawnProfile.generated.h"

/** Species settings partition the existing roster; they never create extra animals. Distances are cm. */
USTRUCT(BlueprintType)
struct FPPAnimalSpawnProfile
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag Species;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPredator = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHerding = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="12")) int32 MinGroupSize = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="12")) int32 MaxGroupSize = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float SpreadRadius = 2000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float MemberSpacing = 800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float GroupSpacing = 6000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float PlayerSafeDistance = 3500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float CampSafeDistance = 3500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float ThreatSeparation = 4000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float RoamRadius = 4000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float RegroupDistance = 3000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="100")) float AlertDistance = 3000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float CalmSeconds = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.5")) float UpdateSeconds = 1.0f;
};
