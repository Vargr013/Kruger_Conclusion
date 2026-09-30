#include "Data/PPAudioSubsystem.h"
#include "Characters/ARangerCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

bool UPPAudioSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type == EWorldType::Game || Type == EWorldType::PIE;
}
TStatId UPPAudioSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UPPAudioSubsystem, STATGROUP_Tickables); }
void UPPAudioSubsystem::OnWorldBeginPlay(UWorld& World)
{
    Super::OnWorldBeginPlay(World);
    const TArray<FString> Names = {TEXT("Wind"), TEXT("River"), TEXT("Spray"), TEXT("Radio"), TEXT("Equipment"), TEXT("Confirm"), TEXT("Miss"),
        TEXT("Grass0"), TEXT("Grass1"), TEXT("Grass2"), TEXT("Sand0"), TEXT("Sand1"), TEXT("Sand2"), TEXT("Stone0"), TEXT("Stone1"), TEXT("Stone2"), TEXT("Wood0"), TEXT("Wood1"), TEXT("Wood2")};
    for (const FString& Name : Names)
    {
        const FString Path = FString::Printf(TEXT("/Game/Poaching_Patrol/Audio/%s.%s"), *Name, *Name);
        if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path)) Sounds.Add(FName(*Name), Sound);
        else UE_LOG(LogTemp, Warning, TEXT("Patrol audio asset missing: %s"), *Path);
    }
    Wind = UGameplayStatics::SpawnSound2D(this, Sounds.FindRef(TEXT("Wind")), 0.001f, 1, 0, nullptr, false, false);
    River = UGameplayStatics::SpawnSound2D(this, Sounds.FindRef(TEXT("River")), 0.001f, 1, 0, nullptr, false, false);
}
void UPPAudioSubsystem::Deinitialize()
{
    for (UAudioComponent* Component : {Wind.Get(), River.Get(), Feedback.Get(), Equipment.Get(), Footstep.Get()})
        if (IsValid(Component)) { Component->Stop(); Component->DestroyComponent(); }
    Sounds.Reset(); LastCueTimes.Reset();
    Super::Deinitialize();
}
void UPPAudioSubsystem::Play(const UObject* Context, FName Cue, bool bUI)
{
    UWorld* World = Context ? Context->GetWorld() : nullptr;
    if (World) if (auto* Audio = World->GetSubsystem<UPPAudioSubsystem>()) Audio->PlayCue(Cue, bUI);
}
void UPPAudioSubsystem::PlayCue(FName Cue, bool bUI)
{
    if (!bUI && GetWorld()->IsPaused()) return;
    USoundBase* Sound = Sounds.FindRef(Cue);
    if (!Sound) return;
    const double Now = FPlatformTime::Seconds();
    const double* Last = LastCueTimes.Find(Cue);
    if (Last && Now - *Last < 0.2) return;
    LastCueTimes.Add(Cue, Now);
    const bool bFeedback = Cue == TEXT("Confirm") || Cue == TEXT("Miss") || Cue == TEXT("Radio");
    UAudioComponent* Previous = bFeedback ? Feedback.Get() : Equipment.Get();
    if (IsValid(Previous)) Previous->Stop();
    const float Gain = Cue == TEXT("Spray") ? 0.22f : Cue == TEXT("Radio") ? 0.08f : 0.12f;
    UAudioComponent* Component = UGameplayStatics::SpawnSound2D(this, Sound, Gain, 1, 0, nullptr, false, true);
    if (Component) Component->bIsUISound = bUI;
    if (bFeedback) Feedback = Component; else Equipment = Component;
}
void UPPAudioSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    ARangerCharacter* Ranger = PC ? Cast<ARangerCharacter>(PC->GetPawn()) : nullptr;
    const bool bGameplay = Ranger && !World->IsPaused() && !PC->IsMoveInputIgnored() && !PC->bShowMouseCursor;
    float WaterDistance = TNumericLimits<float>::Max();
    if (bGameplay)
    {
        // The river is painted landscape water, not a separate water mesh.
        // Sample nearby collision materials twice per second; reuse the result between samples.
        if (World->GetTimeSeconds() >= NextWaterScan)
        {
            NextWaterScan = World->GetTimeSeconds() + 0.5;
            CachedWaterDistance = TNumericLimits<float>::Max();
            CachedSurface = TEXT("Grass");
            FCollisionQueryParams Query(SCENE_QUERY_STAT(PatrolAudioSurface), false, Ranger);
            Query.bReturnPhysicalMaterial = true;
            for (int32 Sample = 0; Sample < 17; ++Sample)
            {
                const float Radius = Sample == 0 ? 0.0f : Sample <= 8 ? 600.0f : 1500.0f;
                const float Angle = (Sample - 1) % 8 * PI / 4.0f;
                const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0);
                const FVector Position = Ranger->GetActorLocation() + Offset;
                FHitResult Hit;
                if (!World->LineTraceSingleByChannel(Hit, Position + FVector(0, 0, 500), Position - FVector(0, 0, 3000), ECC_Visibility, Query)) continue;
                const FString Material = Hit.PhysMaterial.IsValid() ? Hit.PhysMaterial->GetName() : FString();
                if (Material.Contains(TEXT("Water"))) CachedWaterDistance = FMath::Min(CachedWaterDistance, Radius);
                if (Sample == 0)
                {
                    if (Material.Contains(TEXT("Sand")) || Material.Contains(TEXT("Dirt")) || Material.Contains(TEXT("Water"))) CachedSurface = TEXT("Sand");
                    else if (Material.Contains(TEXT("Rock")) || Material.Contains(TEXT("Stone"))) CachedSurface = TEXT("Stone");
                    if (auto* Floor = Cast<UStaticMeshComponent>(Hit.GetComponent()))
                    {
                        FString Name = Floor->GetStaticMesh() ? Floor->GetStaticMesh()->GetName() : FString();
                        for (int32 i = 0; i < Floor->GetNumMaterials(); ++i)
                            if (auto* Mat = Floor->GetMaterial(i)) Name += Mat->GetName();
                        if (Name.Contains(TEXT("Wood")) || Name.Contains(TEXT("Tower"))) CachedSurface = TEXT("Wood");
                        else if (Name.Contains(TEXT("Rock")) || Name.Contains(TEXT("Stone"))) CachedSurface = TEXT("Stone");
                    }
                }
            }
        }
        WaterDistance = CachedWaterDistance;
    }
    const float BlendDelta = FMath::Clamp(DeltaTime, 0.0f, 0.1f);
    WindGain = FMath::FInterpTo(WindGain, bGameplay ? 0.045f : 0.0f, BlendDelta, 1.5f);
    RiverGain = FMath::FInterpTo(RiverGain, bGameplay ? 0.075f * FMath::Clamp(1.0f - WaterDistance / 2200.0f, 0.0f, 1.0f) : 0.0f, BlendDelta, 1.5f);
    if (Wind) Wind->SetVolumeMultiplier(World->IsPaused() ? 0.0f : WindGain);
    if (River) River->SetVolumeMultiplier(World->IsPaused() ? 0.0f : RiverGain);
    const auto* Movement = Ranger ? Ranger->GetCharacterMovement() : nullptr;
    const float Speed = Ranger ? Ranger->GetVelocity().Size2D() : 0;
    if (!bGameplay || !Movement || !Movement->IsMovingOnGround() || Speed < 30)
    {
        StepElapsed = 0;
        if (IsValid(Footstep)) Footstep->Stop();
        return;
    }
    StepElapsed += DeltaTime;
    const float Interval = FMath::Clamp(230.0f / Speed, 0.34f, 0.6f);
    if (StepElapsed < Interval) return;
    StepElapsed = 0; // No catch-up bursts after hitches.
    const FString Surface = CachedSurface;
    const FName Cue(*FString::Printf(TEXT("%s%d"), *Surface, StepIndex++ % 3));
    if (IsValid(Footstep)) Footstep->Stop();
    Footstep = UGameplayStatics::SpawnSound2D(this, Sounds.FindRef(Cue), 0.12f, FMath::FRandRange(0.97f, 1.03f));
}
