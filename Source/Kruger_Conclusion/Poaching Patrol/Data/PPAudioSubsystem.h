#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PPAudioSubsystem.generated.h"
class USoundBase;
class UAudioComponent;

/** Quiet local-player mix. World ownership prevents loops surviving level travel. */
UCLASS()
class KRUGER_CONCLUSION_API UPPAudioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
    virtual void OnWorldBeginPlay(UWorld& World) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickableWhenPaused() const override { return true; }
    virtual TStatId GetStatId() const override;
    static void Play(const UObject* Context, FName Cue, bool bUI = false);
private:
    UPROPERTY() TMap<FName, TObjectPtr<USoundBase>> Sounds;
    UPROPERTY() TObjectPtr<UAudioComponent> Wind;
    UPROPERTY() TObjectPtr<UAudioComponent> River;
    UPROPERTY() TObjectPtr<UAudioComponent> Feedback;
    UPROPERTY() TObjectPtr<UAudioComponent> Equipment;
    UPROPERTY() TObjectPtr<UAudioComponent> Footstep;
    float CachedWaterDistance = TNumericLimits<float>::Max();
    FString CachedSurface = TEXT("Grass");
    TMap<FName, double> LastCueTimes;
    double NextWaterScan = 0;
    float StepElapsed = 0;
    float WindGain = 0;
    float RiverGain = 0;
    int32 StepIndex = 0;
    void PlayCue(FName Cue, bool bUI);
};
