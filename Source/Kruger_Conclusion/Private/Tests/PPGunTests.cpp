#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "BaseGun.h"
#include "BaseProjectile.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/ARangerCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPGunCooldownTest,
    "KrugerConclusion.PoachingPatrol.Tools.ShotCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPGunCooldownTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ABaseGun* Gun = World->SpawnActor<ABaseGun>();
    Gun->FireMode = EFireMode::Raycast;
    Gun->Shoot();
    TestEqual(TEXT("The first shot was immediate"), Gun->GetRemainingUses(), 9);
    for (int32 Click = 0; Click < 20; ++Click) Gun->Shoot();
    TestEqual(TEXT("Rapid clicks did not spend ammunition"), Gun->GetRemainingUses(), 9);
    World->Tick(LEVELTICK_All, 0.2f);
    Gun->Shoot();
    TestEqual(TEXT("The cooldown blocked an early shot"), Gun->GetRemainingUses(), 9);
    Gun->Reload();
    Gun->Shoot();
    TestEqual(TEXT("Refilling did not reset the cooldown"), Gun->GetRemainingUses(), 10);
    ++GFrameCounter;
    World->Tick(LEVELTICK_All, 0.25f);
    Gun->Shoot();
    TestEqual(TEXT("A new shot was accepted after the delay"), Gun->GetRemainingUses(), 9);
    for (int32 Shot = 0; Shot < 12; ++Shot)
    {
        ++GFrameCounter;
        World->Tick(LEVELTICK_All, 0.25f);
        ++GFrameCounter;
        World->Tick(LEVELTICK_All, 0.25f);
        Gun->Shoot();
    }
    TestEqual(TEXT("Empty ammunition stayed at zero"), Gun->GetRemainingUses(), 0);
    ABaseGun* ProjectileGun = World->SpawnActor<ABaseGun>();
    ProjectileGun->Shoot();
    TestEqual(TEXT("An unconfigured projectile did not use ammunition"), ProjectileGun->GetRemainingUses(), 10);
    ProjectileGun->ProjectileClass = ABaseProjectile::StaticClass();
    ProjectileGun->Shoot();
    TestEqual(TEXT("A spawned projectile used one round"), ProjectileGun->GetRemainingUses(), 9);
    ProjectileGun->Shoot();
    TestEqual(TEXT("Projectiles used the same cooldown"), ProjectileGun->GetRemainingUses(), 9);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPGunCrosshairAimTest,
    "KrugerConclusion.PoachingPatrol.Tools.CrosshairAim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPGunCrosshairAimTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    ARangerCharacter* Ranger = World->SpawnActor<ARangerCharacter>();
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    ACameraActor* Camera = World->SpawnActor<ACameraActor>();
    Controller->Possess(Ranger);
    Controller->SetViewTarget(Camera);
    Controller->PlayerCameraManager->bUseClientSideCameraUpdates = false;
    // Advance the camera cache so shots use the rendered view, including pitch and roll.
    World->Tick(LEVELTICK_All, 0.01f);

    const FVector ViewLocation(0.0f, 0.0f, 200.0f);
    auto FireAtCrosshair = [&](float Distance, const FRotator& ViewRotation, bool bBlocked, bool bUpgraded)
    {
        Camera->SetActorLocationAndRotation(ViewLocation, ViewRotation);
        Controller->PlayerCameraManager->UpdateCamera(0.0f);
        const FVector Muzzle = ViewLocation + ViewRotation.RotateVector(FVector(100.0f, 40.0f, -30.0f));
        ABaseGun* Gun = World->SpawnActor<ABaseGun>();
        Gun->SetOwner(Ranger);
        Gun->SetActorLocationAndRotation(Muzzle, ViewRotation + FRotator(0.0f, 5.0f, 0.0f));
        Gun->FireMode = EFireMode::Raycast;
        if (bUpgraded) Gun->ApplyRangeUpgrade();

        APPPoacherCharacter* Target = World->SpawnActor<APPPoacherCharacter>();
        Target->GetCapsuleComponent()->SetCapsuleSize(12.0f, 60.0f);
        Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Target->SetActorLocation(ViewLocation + ViewRotation.Vector() * Distance);

        UBoxComponent* Blocker = nullptr;
        AActor* Wall = nullptr;
        if (bBlocked)
        {
            Wall = World->SpawnActor<AActor>();
            Blocker = NewObject<UBoxComponent>(Wall);
            Wall->SetRootComponent(Blocker);
            Blocker->SetBoxExtent(FVector(4.0f, 4.0f, 4.0f));
            Blocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
            Blocker->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
            Blocker->RegisterComponent();
            // This cover blocks the muzzle path while leaving the camera's sightline clear.
            Blocker->SetWorldLocation(FMath::Lerp(Muzzle, Target->GetActorLocation(), 0.25f));
        }

        Gun->Shoot();
        const bool bSprayed = Target->IsPepperSprayed();
        if (Wall) Wall->Destroy();
        Target->Destroy();
        Gun->Destroy();
        return bSprayed;
    };

    TestTrue(TEXT("Offset muzzle hits the close crosshair target"), FireAtCrosshair(220.0f, FRotator::ZeroRotator, false, false));
    TestTrue(TEXT("Offset muzzle hits the crosshair at a farther distance"), FireAtCrosshair(500.0f, FRotator::ZeroRotator, false, false));
    TestTrue(TEXT("Pitched and rolled camera still aims at the crosshair"), FireAtCrosshair(400.0f, FRotator(-20.0f, 35.0f, 4.0f), false, false));
    TestFalse(TEXT("Nearby muzzle cover blocks a visible target"), FireAtCrosshair(400.0f, FRotator::ZeroRotator, true, false));
    TestFalse(TEXT("Crosshair aim does not extend the spray range"), FireAtCrosshair(750.0f, FRotator::ZeroRotator, false, false));
    TestTrue(TEXT("Range upgrade reaches the farther crosshair target"), FireAtCrosshair(750.0f, FRotator::ZeroRotator, false, true));

    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}

#endif
