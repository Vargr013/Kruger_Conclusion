#pragma once

#include "CoreMinimal.h"
#include "PPGameTypes.generated.h"

class APPPoacherCharacter;

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

UENUM(BlueprintType)
enum class EPPObjectiveKind : uint8
{
	ArrestQuota UMETA(DisplayName="Arrest Quota"),
	ResolveRemaining UMETA(DisplayName="Resolve Remaining"),
	EscortToArrestZone UMETA(DisplayName="Escort To Arrest Zone"),
	ConservationStatus UMETA(DisplayName="Conservation Status")
};

UENUM(BlueprintType)
enum class EPPObjectiveProgressState : uint8
{
	Active UMETA(DisplayName="Active"),
	Completed UMETA(DisplayName="Completed"),
	Failed UMETA(DisplayName="Failed")
};

UENUM(BlueprintType)
enum class EPPRoundOutcome : uint8
{
	InProgress UMETA(DisplayName="In Progress"),
	Success UMETA(DisplayName="Success"),
	Failure UMETA(DisplayName="Failure")
};

USTRUCT(BlueprintType)
struct KRUGER_CONCLUSION_API FPPObjectiveState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Objective")
	FName Identifier = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Objective")
	EPPObjectiveKind Kind = EPPObjectiveKind::ArrestQuota;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Objective")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Objective")
	FText Detail;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Objective")
	int32 CurrentValue = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Objective")
	int32 TargetValue = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Objective")
	bool bRequired = true;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Objective")
	EPPObjectiveProgressState ProgressState = EPPObjectiveProgressState::Active;
};

USTRUCT(BlueprintType)
struct KRUGER_CONCLUSION_API FPPEscortStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Escort")
	TObjectPtr<APPPoacherCharacter> SelectedPoacher = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Escort")
	int32 ActiveEscortCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Escort")
	float DistanceToCaptor = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Escort")
	float SafeRange = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Escort")
	float EscapeProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Escort")
	float EscapeGraceTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Escort")
	float SecondsRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Escort")
	bool bUnderEscapePressure = false;

	float GetNormalizedEscapeProgress() const
	{
		return EscapeGraceTime > KINDA_SMALL_NUMBER
			? FMath::Clamp(EscapeProgress / EscapeGraceTime, 0.0f, 1.0f)
			: (bUnderEscapePressure ? 1.0f : 0.0f);
	}
};

USTRUCT(BlueprintType)
struct KRUGER_CONCLUSION_API FPPRoundSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	int32 TotalPoachers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	int32 PoachersArrested = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	int32 PoachersPermanentlyEscaped = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	int32 ActivePoachers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	int32 TotalAnimals = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	int32 AnimalsAlive = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	int32 AnimalsPoached = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	int32 RequiredArrests = 0;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	float CaptureRate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	bool bQuotaMet = false;
};

USTRUCT(BlueprintType)
struct KRUGER_CONCLUSION_API FPPRoundResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	EPPRoundOutcome Outcome = EPPRoundOutcome::InProgress;

	UPROPERTY(BlueprintReadOnly, Category="Poaching Patrol|Round")
	FPPRoundSnapshot Snapshot;
};
