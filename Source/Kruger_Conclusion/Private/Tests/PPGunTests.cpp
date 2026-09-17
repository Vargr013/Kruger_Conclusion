#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "BaseGun.h"
#include "BaseProjectile.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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

#endif
