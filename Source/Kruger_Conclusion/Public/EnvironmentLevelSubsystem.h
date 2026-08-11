#pragma once

#include "CoreMinimal.h"
#include "Data/PPGameTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnvironmentLevelSubsystem.generated.h"

class AActor;
class APPAnimalCharacter;
class APPPoacherCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelStateChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPPRoundStateChangedSignature, FPPRoundSnapshot, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPPRoundEndedSignature, FPPRoundResult, Result);

UCLASS(Config=Game)
class KRUGER_CONCLUSION_API UEnvironmentLevelSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	void RegisterPoacher(APPPoacherCharacter* Poacher);
	void RegisterAnimal(APPAnimalCharacter* Animal);
	void ReportPoacherArrested(APPPoacherCharacter* Poacher);
	void ReportPoacherPermanentlyEscaped(APPPoacherCharacter* Poacher);
	void ReportAnimalPoached(APPAnimalCharacter* Animal);

	// Compatibility hooks for existing Blueprints. New gameplay code should use actor-aware Report methods.
	UFUNCTION(BlueprintCallable, Category="EcoSystem|LevelRules", meta=(DeprecatedFunction, DeprecationMessage="Use ReportPoacherArrested with the poacher actor."))
	void OnPoacherCaptured();

	UFUNCTION(BlueprintCallable, Category="EcoSystem|LevelRules", meta=(DeprecatedFunction, DeprecationMessage="Use ReportPoacherPermanentlyEscaped with the poacher actor."))
	void OnPoacherEscaped();

	UFUNCTION(BlueprintCallable, Category="EcoSystem|LevelRules", meta=(DeprecatedFunction, DeprecationMessage="Use ReportAnimalPoached with the animal actor."))
	void OnAnimalPoached();

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Round")
	FPPRoundSnapshot GetRoundSnapshot() const;

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Round")
	FPPRoundResult GetFinalRoundResult() const { return FinalRoundResult; }

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Round")
	bool HasRoundEnded() const { return bRoundEnded; }

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Objectives")
	TArray<FPPObjectiveState> GetObjectivesForPlayer(AActor* Captor) const;

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Escort")
	bool GetMostUrgentEscortStatus(AActor* Captor, FPPEscortStatus& OutStatus) const;

	static int32 CalculateRequiredArrests(int32 TotalPoachers, float Threshold);

	UPROPERTY(BlueprintAssignable, Category="Poaching Patrol|Events")
	FOnPPRoundStateChangedSignature OnRoundStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Poaching Patrol|Events")
	FOnPPRoundEndedSignature OnRoundEnded;

	UPROPERTY(BlueprintAssignable, Category="EcoSystem|Events")
	FOnLevelStateChangedSignature OnLevelWon;

	UPROPERTY(BlueprintAssignable, Category="EcoSystem|Events")
	FOnLevelStateChangedSignature OnLevelLost;

private:
	void ResetRoundState();
	void ScanPlacedActors();
	void BroadcastStateChanged();
	void CheckWinCondition();

	UPROPERTY(Config, EditAnywhere, Category="Poaching Patrol|Round", meta=(ClampMin="0.0", ClampMax="1.0"))
	float WinThresholdPercentage = 0.50f;

	TSet<TWeakObjectPtr<APPPoacherCharacter>> RegisteredPoachers;
	TSet<TWeakObjectPtr<APPAnimalCharacter>> RegisteredAnimals;
	TSet<TWeakObjectPtr<APPPoacherCharacter>> ArrestedPoachers;
	TSet<TWeakObjectPtr<APPPoacherCharacter>> PermanentlyEscapedPoachers;
	TSet<TWeakObjectPtr<APPAnimalCharacter>> PoachedAnimals;

	UPROPERTY()
	FPPRoundResult FinalRoundResult;

	bool bRoundEnded = false;
};
