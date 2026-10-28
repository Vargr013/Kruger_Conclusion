#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Data/PPHealthComponent.h"
#include "EnvironmentLevelSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"

#if WITH_EDITOR
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "UObject/UnrealType.h"
#endif

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
	Poacher->StopAIUpdates();
	TestFalse(TEXT("Stopping AI cancelled the prepared animal attack"), Poacher->IsAnimalAttackPending());
	Poacher->UpdateCreatureAI();
	TestTrue(TEXT("Resuming the hunt prepared a fresh attack"), Poacher->IsAnimalAttackPending());
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

#if WITH_EDITOR
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FPPFleeingAnimalHuntingTest,
	"KrugerConclusion.PoachingPatrol.AI.FleeingAnimalHunting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FPPFleeingAnimalHuntingTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	Names.Add(TEXT("GameplayPoacher"));
	Commands.Add(TEXT("MyPPPoacherCharacter"));
	Names.Add(TEXT("PlacedPoacher"));
	Commands.Add(TEXT("MyPPPoacherCharacter_Test"));
}

bool FPPFleeingAnimalHuntingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto Cleanup = [World]()
	{
		World->EndPlay(EEndPlayReason::Quit);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};

	// Build a flat, temporary navigation lane; neither character is teleported during the chase.
	AActor* Floor = World->SpawnActor<AActor>();
	UStaticMeshComponent* FloorMesh = NewObject<UStaticMeshComponent>(Floor);
	Floor->SetRootComponent(FloorMesh);
	FloorMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	Floor->SetActorLocation(FVector(6000, 0, -50));
	Floor->SetActorScale3D(FVector(160, 40, 1));
	FloorMesh->RegisterComponent();
	ANavMeshBoundsVolume* Bounds = World->SpawnActor<ANavMeshBoundsVolume>(
		ANavMeshBoundsVolume::StaticClass(), FVector(6000, 0, 0), FRotator::ZeroRotator);
	UCubeBuilder* Builder = NewObject<UCubeBuilder>();
	Builder->X = 16000;
	Builder->Y = 4000;
	Builder->Z = 1000;
	UActorFactory::CreateBrushForVolumeActor(Bounds, Builder);
	if (!TestTrue(TEXT("Navigation bounds were built"), Bounds->GetComponentsBoundingBox(true).GetSize().X > 15000))
	{
		Cleanup();
		return false;
	}
	FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::GameMode);
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!TestNotNull(TEXT("Navigation system exists"), Navigation))
	{
		Cleanup();
		return false;
	}
	Navigation->OnNavigationBoundsUpdated(Bounds);
	Navigation->Tick(0.1f);
	ANavigationData* NavData = Navigation->GetDefaultNavDataInstance(FNavigationSystem::Create);
	if (!TestNotNull(TEXT("Chase navigation data exists"), NavData))
	{
		Cleanup();
		return false;
	}
	// The shipped map uses prebuilt navigation; this temporary world needs runtime generation.
	FEnumProperty* RuntimeGeneration = FindFProperty<FEnumProperty>(ANavigationData::StaticClass(), TEXT("RuntimeGeneration"));
	if (!TestNotNull(TEXT("Runtime navigation setting exists"), RuntimeGeneration))
	{
		Cleanup();
		return false;
	}
	RuntimeGeneration->GetUnderlyingProperty()->SetIntPropertyValue(
		RuntimeGeneration->ContainerPtrToValuePtr<void>(NavData), static_cast<int64>(ERuntimeGenerationType::Dynamic));
	NavData->ConditionalConstructGenerator();
	Navigation->Build();
	NavData->EnsureBuildCompletion();
	FNavLocation Start;
	if (!TestTrue(TEXT("Chase lane is navigable"), Navigation->ProjectPointToNavigation(FVector(2500, 0, 100), Start)))
	{
		Cleanup();
		return false;
	}

	const FString PoacherPath = FString::Printf(TEXT("/Game/Poaching_Patrol/Characters/Poachers/%s.%s_C"), *Parameters, *Parameters);
	UClass* PoacherClass = LoadClass<APPPoacherCharacter>(nullptr, *PoacherPath);
	UClass* AnimalClass = LoadClass<APPAnimalCharacter>(nullptr,
		TEXT("/Game/Poaching_Patrol/Characters/Animals/MyPPAnimalCharacter_Elephant.MyPPAnimalCharacter_Elephant_C"));
	if (!TestNotNull(TEXT("Gameplay poacher Blueprint loaded"), PoacherClass)
		|| !TestNotNull(TEXT("Elephant Blueprint loaded"), AnimalClass))
	{
		Cleanup();
		return false;
	}
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APPPoacherCharacter* Poacher = World->SpawnActor<APPPoacherCharacter>(PoacherClass, FVector(2500, 0, 160), FRotator::ZeroRotator, Spawn);
	APPAnimalCharacter* Animal = World->SpawnActor<APPAnimalCharacter>(AnimalClass, FVector(3500, 0, 160), FRotator(0, 180, 0), Spawn);
	Poacher->DispatchBeginPlay();
	Animal->DispatchBeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	const FVector AnimalStart = Animal->GetActorLocation();
	const FVector PoacherStart = Poacher->GetActorLocation();
	float FurthestAnimalX = AnimalStart.X;
	bool bSawFleeing = false;
	bool bSawWarning = false;
	bool bMovedDuringWarning = false;
	bool bPassedOldLeash = false;
	bool bHuntedBeyondOldLeash = false;
	UBoxComponent* SightBlocker = nullptr;
	for (int32 Step = 0; Step < 900 && !Animal->GetHealthComponent()->IsDead(); ++Step)
	{
		if (Step == 30)
		{
			AActor* Cover = World->SpawnActor<AActor>();
			SightBlocker = NewObject<UBoxComponent>(Cover);
			Cover->SetRootComponent(SightBlocker);
			SightBlocker->SetBoxExtent(FVector(20, 1000, 500));
			SightBlocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			SightBlocker->SetCollisionResponseToAllChannels(ECR_Ignore);
			SightBlocker->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			SightBlocker->SetCanEverAffectNavigation(false);
			SightBlocker->RegisterComponent();
			Cover->SetActorLocation((Poacher->GetActorLocation() + Animal->GetActorLocation()) * 0.5f);
			TestFalse(TEXT("Brief cover genuinely blocked sight"), Poacher->CanSeeTarget(Animal));
		}
		if (Step == 45)
		{
			TestEqual(TEXT("Lost animal sight entered investigation"), Poacher->GetPoacherState(), EPPPoacherState::Investigating);
			TestTrue(TEXT("Investigation retained the animal observation"), Poacher->GetLastObservedTime() > 0);
			SightBlocker->DestroyComponent();
		}
		const bool bWasPreparing = Poacher->IsAnimalAttackPending();
		const FVector PreviousPoacherLocation = Poacher->GetActorLocation();
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 1.0f / 30.0f);
		bSawFleeing |= Animal->GetAnimalState() == EPPAnimalState::Fleeing;
		bSawWarning |= Poacher->IsAnimalAttackPending();
		bMovedDuringWarning |= bWasPreparing && FVector::Dist2D(PreviousPoacherLocation, Poacher->GetActorLocation()) > 1.0f;
		FurthestAnimalX = FMath::Max(FurthestAnimalX, Animal->GetActorLocation().X);
		bPassedOldLeash |= Animal->GetActorLocation().X - PoacherStart.X > 3000;
		bHuntedBeyondOldLeash |= bPassedOldLeash && Poacher->GetTargetActor() == Animal;
	}
	TestTrue(TEXT("The animal detected the poacher and fled"), bSawFleeing);
	TestTrue(TEXT("The fleeing animal moved at least ten metres"), FurthestAnimalX - AnimalStart.X > 1000);
	TestTrue(TEXT("The poacher pursued using character movement"), Poacher->GetActorLocation().X - PoacherStart.X > 1000);
	TestTrue(TEXT("The hunt continued beyond the old thirty-metre home leash"), bHuntedBeyondOldLeash);
	TestTrue(TEXT("Animal attacks retained a preparation window"), bSawWarning);
	TestTrue(TEXT("The poacher kept closing distance during preparation"), bMovedDuringWarning);
	TestTrue(TEXT("An unattended fleeing elephant was killed within thirty seconds"), Animal->GetHealthComponent()->IsDead());
	TestEqual(TEXT("The moving animal loss was recorded once"), World->GetSubsystem<UEnvironmentLevelSubsystem>()->GetRoundSnapshot().AnimalsPoached, 1);
	AddInfo(FString::Printf(TEXT("Fleeing hunt: elapsed=%.2fs, animal travel=%.0fcm, poacher travel=%.0fcm"),
		World->GetTimeSeconds(), FurthestAnimalX - AnimalStart.X, Poacher->GetActorLocation().X - PoacherStart.X));
	Cleanup();
	return true;
}
#endif

#endif
