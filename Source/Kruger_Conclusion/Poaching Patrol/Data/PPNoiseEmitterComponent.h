#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PPNoiseEmitterComponent.generated.h"

USTRUCT(BlueprintType)
struct FPPNoiseStimulus
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) float Radius = 0.0f;
	UPROPERTY(BlueprintReadOnly) float Time = -1000.0f;
	UPROPERTY(BlueprintReadOnly) float Lifetime = 0.75f;
	bool IsActive(float Now) const { return Radius > 0.0f && Now >= Time && Now - Time <= Lifetime; }
};

/** Gameplay noise stays independent of the player's audio mix. */
UCLASS(ClassGroup=(Patrol), meta=(BlueprintSpawnableComponent))
class KRUGER_CONCLUSION_API UPPNoiseEmitterComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPPNoiseEmitterComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise") float FastMovementSpeedThreshold = 400.0f;
	UFUNCTION(BlueprintCallable, Category="Noise") void EmitNoise(float Radius);
	UFUNCTION(BlueprintCallable, Category="Noise") void ClearNoise();
	void GetActiveStimuli(TArray<FPPNoiseStimulus>& OutStimuli) const;
	static float MovementRadius(bool bCrouched, bool bFast, float Speed);
private:
	UPROPERTY(Transient) FPPNoiseStimulus MovementNoise;
	UPROPERTY(Transient) FPPNoiseStimulus ActionNoise;
	float NextStepTime = 0.0f;
};
