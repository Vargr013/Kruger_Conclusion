#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Data/PPHealthComponent.h"
#include "EnvironmentLevelSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPAnimalHuntingTest,
	"KrugerConclusion.PoachingPatrol.AI.AnimalHunting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPAnimalHuntingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	UClass* PoacherClass = LoadClass<APPPoacherCharacter>(nullptr, TEXT("/Game/Poaching_Patrol/Characters/Poachers/MyPPPoacherCharacter.MyPPPoacherCharacter_C"));
	UClass* AnimalClass = LoadClass<APPAnimalCharacter>(nullptr, TEXT("/Game/Poaching_Patrol/Characters/Animals/MyPPAnimalCharacter_Elephant.MyPPAnimalCharacter_Elephant_C"));
	if (!TestNotNull(TEXT("Gameplay poacher Blueprint loaded"), PoacherClass) || !TestNotNull(TEXT("Elephant Blueprint loaded"), AnimalClass))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APPPoacherCharacter* Poacher = World->SpawnActor<APPPoacherCharacter>(PoacherClass, FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	APPAnimalCharacter* Animal = World->SpawnActor<APPAnimalCharacter>(AnimalClass, FVector(500, 0, 0), FRotator::ZeroRotator, Spawn);
	Poacher->DispatchBeginPlay();
	Animal->DispatchBeginPlay();
	TestTrue(TEXT("Animal began play"), Animal->HasActorBegunPlay());
	TestTrue(TEXT("Animal death callback was bound"), Animal->GetHealthComponent()->OnDeath.IsBound());
	Poacher->StopAIUpdates();
	Animal->StopAIUpdates();
	Poacher->GetCharacterMovement()->DisableMovement();
	Animal->GetCharacterMovement()->DisableMovement();
	Animal->SetActorLocation(FVector(260, 0, 0));
	const float StartingHealth = Animal->GetHealthComponent()->GetCurrentHealth();
	Poacher->UpdateCreatureAI();
	TestTrue(TEXT("Hunting speed could catch the 420 cm/s elephant"), Poacher->GetCharacterMovement()->MaxWalkSpeed > 420.0f);
	TestEqual(TEXT("Attack preparation did not instantly damage the animal"), Animal->GetHealthComponent()->GetCurrentHealth(), StartingHealth);
	for (int32 Step = 0; Step < 115; ++Step)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.1f);
		Poacher->UpdateCreatureAI();
	}
	TestTrue(TEXT("An unattended reachable elephant was killed"), Animal->GetHealthComponent()->IsDead());
	UEnvironmentLevelSubsystem* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	TestEqual(TEXT("The loss was recorded once"), Rules->GetRoundSnapshot().AnimalsPoached, 1);
	for (int32 Step = 0; Step < 30; ++Step)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.1f);
	}
	APPAnimalCharacter* ProtectedAnimal = World->SpawnActor<APPAnimalCharacter>(AnimalClass, FVector(260, 0, 0), FRotator::ZeroRotator, Spawn);
	ProtectedAnimal->DispatchBeginPlay();
	ProtectedAnimal->StopAIUpdates();
	ProtectedAnimal->GetCharacterMovement()->DisableMovement();
	Poacher->UpdateCreatureAI();
	TestTrue(TEXT("The next attack had a warning period"), Poacher->IsAnimalAttackPending());
	Poacher->EnterSubduedState(0.0f);
	TestFalse(TEXT("Subdual cancelled the prepared attack"), Poacher->IsAnimalAttackPending());
	for (int32 Step = 0; Step < 20; ++Step)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.1f);
		Poacher->UpdateCreatureAI();
	}
	TestEqual(TEXT("Intervention protected the animal"), ProtectedAnimal->GetHealthComponent()->GetCurrentHealth(), StartingHealth);
	UGameplayStatics::ApplyDamage(ProtectedAnimal, StartingHealth, nullptr, ProtectedAnimal, nullptr);
	TestEqual(TEXT("Other causes still counted as wildlife losses"), Rules->GetRoundSnapshot().AnimalsLost, 2);
	TestEqual(TEXT("Other causes did not count as poacher kills"), Rules->GetRoundSnapshot().AnimalsPoached, 1);
	Rules->ReportAnimalLost(ProtectedAnimal, false);
	TestEqual(TEXT("Repeated wildlife reports were ignored"), Rules->GetRoundSnapshot().AnimalsLost, 2);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
