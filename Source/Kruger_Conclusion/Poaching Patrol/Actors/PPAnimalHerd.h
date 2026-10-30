#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/PPAnimalSpawnProfile.h"
#include "PPAnimalHerd.generated.h"

class APPSpawnDirector;
class APPAnimalCharacter;

/** A small group shares destinations; each animal keeps its own threat and movement state. */
UCLASS()
class KRUGER_CONCLUSION_API APPAnimalHerd : public AActor
{
	GENERATED_BODY()
public:
	APPAnimalHerd();
	void Initialise(APPSpawnDirector* Director, const FVector& Center, const FPPAnimalSpawnProfile& Settings,
		const TArray<APPAnimalCharacter*>& Animals, int32 Seed);
	void RemoveMember(APPAnimalCharacter* Animal);
	void ReportThreat(APPAnimalCharacter* Observer, const FVector& LastKnownLocation);
	bool GetRoamGoal(APPAnimalCharacter* Animal, FVector& OutGoal) const;
	bool ShouldRegroup(const APPAnimalCharacter* Animal) const;
	bool FindSafeGoal(APPAnimalCharacter* Animal, const FVector& Desired, FVector& OutGoal) const;
	bool GetSeparationGoal(APPAnimalCharacter* Animal, FVector& OutGoal, bool& bCrowded) const;
	void UpdateHerd();
	UFUNCTION(BlueprintPure, Category="Herd") TArray<APPAnimalCharacter*> GetLivingMembers() const;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Herd") FVector Center = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Herd") FPPAnimalSpawnProfile Profile;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	TWeakObjectPtr<APPSpawnDirector> SpawnDirector;
	UPROPERTY(Transient) TArray<TWeakObjectPtr<APPAnimalCharacter>> Members;
	TMap<TWeakObjectPtr<APPAnimalCharacter>, FVector> Offsets;
	TMap<TWeakObjectPtr<APPAnimalCharacter>, FVector> Goals;
	TMap<TWeakObjectPtr<APPAnimalCharacter>, float> AlertTimes;
	FRandomStream Random;
	FVector TerritoryCenter = FVector::ZeroVector;
	FTimerHandle UpdateTimer;
	float NextDestinationTime = 0;
	float LastThreatTime = -1000;
	bool bNeedsSafeRegroup = false;
	bool ChooseDestination(const FVector& DesiredCenter);
};
