#pragma once

#include "CoreMinimal.h"
#include "PPGameTypes.generated.h"

UENUM(BlueprintType)
enum class EAnimalState : uint8
{
    Wandering UMETA(DisplayName="Wandering"),
    Alert UMETA(DisplayName="Alert"),
    Fleeing UMETA(DisplayName="Fleeing"),
    Stalk UMETA(DisplayName="Stalk")
};

UENUM(BlueprintType)
enum class EPoacherState : uint8
{
    Tracking UMETA(DisplayName="Tracking"),
    Engaging UMETA(DisplayName="Engaging"),
    Attacking UMETA(DisplayName="Attacking"),
    Fleeing UMETA(DisplayName="Fleeing"),
    Waiting UMETA(DisplayName="Waiting"),
    Captured UMETA(DisplayName="Captured")
};