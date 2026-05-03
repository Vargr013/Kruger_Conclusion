#pragma once

#include "CoreMinimal.h"
#include "PPGameTypes.generated.h"

UENUM(BlueprintType)
enum class EPPAnimalState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Roaming UMETA(DisplayName="Roaming"),
	Alert UMETA(DisplayName="Alert"),
	Fleeing UMETA(DisplayName="Fleeing")
};

UENUM(BlueprintType)
enum class EPPPoacherState : uint8
{
	DisguisedRoaming UMETA(DisplayName="Disguised Roaming"),
	Alert UMETA(DisplayName="Alert"),
	Fleeing UMETA(DisplayName="Fleeing"),
	Captured UMETA(DisplayName="Captured"),
	FollowingPlayer UMETA(DisplayName="Following Player"),
	Arrested UMETA(DisplayName="Arrested"),
	Escaped UMETA(DisplayName="Escaped")
};
