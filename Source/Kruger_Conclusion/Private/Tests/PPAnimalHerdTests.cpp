#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Actors/PPAnimalHerd.h"
#include "Actors/PPSpawnDirector.h"
#include "Actors/PPArrestZone.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Data/PPHealthComponent.h"
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnvironmentLevelSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPAnimalPopulationPartitionTest, "KrugerConclusion.PoachingPatrol.Animals.PopulationPartition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPAnimalPopulationPartitionTest::RunTest(const FString& Parameters)
{
	for (int32 Seed = 1; Seed <= 50; ++Seed)
	{
		FRandomStream Random(Seed); TArray<int32> Sizes;
		TestTrue(TEXT("Thirteen animals can form herds"), APPSpawnDirector::PartitionPopulation(13, 3, 5, Random, Sizes));
		int32 Total = 0;
		for (int32 Size : Sizes) { Total += Size; TestTrue(TEXT("Each herd respects its range"), Size >= 3 && Size <= 5); }
		TestEqual(TEXT("No animal is added or lost to fill a herd"), Total, 13);
		TestTrue(TEXT("Seven predators can remain solitary"), APPSpawnDirector::PartitionPopulation(7, 1, 1, Random, Sizes));
		TestEqual(TEXT("Seven solitary groups"), Sizes.Num(), 7);
		TestFalse(TEXT("Impossible settings are rejected"), APPSpawnDirector::PartitionPopulation(7, 4, 5, Random, Sizes));
		TestTrue(TEXT("Failed partition exposes no partial groups"), Sizes.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPAnimalLayoutAndHerdTest, "KrugerConclusion.PoachingPatrol.Animals.LayoutAndHerdLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPAnimalLayoutAndHerdTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto Cleanup = [&]() { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AActor* Floor = World->SpawnActor<AActor>();
	UStaticMeshComponent* FloorMesh = NewObject<UStaticMeshComponent>(Floor);
	Floor->SetRootComponent(FloorMesh);
	FloorMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	Floor->SetActorLocation(FVector(30000, 30000, -50)); Floor->SetActorScale3D(FVector(600, 600, 1));
	FloorMesh->RegisterComponent();
	ANavMeshBoundsVolume* Bounds = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), FVector(30000, 30000, 0), FRotator::ZeroRotator);
	UCubeBuilder* Builder = NewObject<UCubeBuilder>(); Builder->X = 60000; Builder->Y = 60000; Builder->Z = 2000;
	UActorFactory::CreateBrushForVolumeActor(Bounds, Builder);
	FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::GameMode);
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(World);
	if (!TestNotNull(TEXT("Population navigation exists"), Navigation)) { Cleanup(); return false; }
	Navigation->OnNavigationBoundsUpdated(Bounds); Navigation->Tick(0.1f);
	ANavigationData* Data = Navigation->GetDefaultNavDataInstance(FNavigationSystem::Create);
	if (!TestNotNull(TEXT("Population navigation data exists"), Data)) { Cleanup(); return false; }
	FEnumProperty* Generation = FindFProperty<FEnumProperty>(ANavigationData::StaticClass(), TEXT("RuntimeGeneration"));
	Generation->GetUnderlyingProperty()->SetIntPropertyValue(Generation->ContainerPtrToValuePtr<void>(Data), static_cast<int64>(ERuntimeGenerationType::Dynamic));
	Data->ConditionalConstructGenerator(); Navigation->Build(); Data->EnsureBuildCompletion();
	APPSpawnDirector* Director = World->SpawnActor<APPSpawnDirector>();
	Director->SetActorLocation(FVector(30000, 30000, 0)); Director->SpawnBounds->SetBoxExtent(FVector(28000, 28000, 900));
	Director->FixedSeed = 42;
	APlayerStart* Player = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(5000, 5000, 100), FRotator::ZeroRotator);
	World->SpawnActor<APPArrestZone>(APPArrestZone::StaticClass(), FVector(8000, 5000, 100), FRotator::ZeroRotator);
	FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TArray<APPPoacherCharacter*> Poachers;
	for (int32 I = 0; I < 4; ++I) Poachers.Add(World->SpawnActor<APPPoacherCharacter>(APPPoacherCharacter::StaticClass(), FVector(10000 + I * 6000, 10000, 100), FRotator::ZeroRotator, Spawn));
	TArray<APPAnimalCharacter*> Animals;
	TArray<FTransform> Originals;
	for (int32 I = 0; I < 20; ++I)
	{
		APPAnimalCharacter* Animal = World->SpawnActor<APPAnimalCharacter>(APPAnimalCharacter::StaticClass(), FVector(5000 + I * 1500, 20000, 100), FRotator::ZeroRotator, Spawn);
		FindFProperty<FBoolProperty>(APPAnimalCharacter::StaticClass(), TEXT("bIsPredator"))->SetPropertyValue_InContainer(Animal, I >= 13);
		*FindFProperty<FStructProperty>(APPAnimalCharacter::StaticClass(), TEXT("AnimalSpeciesTag"))->ContainerPtrToValuePtr<FGameplayTag>(Animal)
			= FGameplayTag::RequestGameplayTag(I >= 13 ? TEXT("Lion") : TEXT("Elephant"));
		Animals.Add(Animal); Originals.Add(Animal->GetActorTransform());
	}
	TArray<FPPSpawnPlacement> PoacherPlan;
	UStaticMeshComponent* WideBody = NewObject<UStaticMeshComponent>(Animals[0]);
	WideBody->SetupAttachment(Animals[0]->GetRootComponent());
	WideBody->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	WideBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WideBody->SetRelativeScale3D(FVector(10, 4, 4)); WideBody->SetRelativeLocation(FVector(0, 0, 150));
	WideBody->RegisterComponent();
	AActor* Obstacle = World->SpawnActor<AActor>();
	UBoxComponent* ObstacleBox = NewObject<UBoxComponent>(Obstacle); Obstacle->SetRootComponent(ObstacleBox);
	ObstacleBox->SetCanEverAffectNavigation(false); ObstacleBox->SetBoxExtent(FVector(30, 30, 100));
	ObstacleBox->SetCollisionProfileName(TEXT("BlockAll")); ObstacleBox->RegisterComponent();
	Obstacle->SetActorLocation(FVector(6400, 30150, 200));
	FVector BodyPosition; FName BodyReason;
	TestFalse(TEXT("Visible body clearance rejects obstacles outside the small capsule"), Director->ProjectCreatureLocation(Animals[0], FVector(6000, 30000, 100), BodyPosition, BodyReason));
	TestEqual(TEXT("The visible body caused the rejection"), BodyReason, FName(TEXT("BlockedBody")));
	const FQuat SideFacing = FRotator(0, 90, 0).Quaternion();
	TestTrue(TEXT("Body clearance respects the animal's facing"), Director->ProjectCreatureLocation(Animals[0], FVector(6000, 30000, 100), BodyPosition, BodyReason, &SideFacing));
	TestFalse(TEXT("A capsule-valid route must also fit the visible body"), Director->HasCreatureRoute(Animals[0], FVector(5000, 30000, 100), FVector(7000, 30000, 100), 5000));
	Obstacle->SetActorLocation(FVector(6400, 30300, 200));
	TestTrue(TEXT("A long body can travel beside an obstacle outside its actual width"), Director->HasCreatureRoute(Animals[0], FVector(5000, 30000, 100), FVector(7000, 30000, 100), 5000));
	WideBody->DestroyComponent(); Obstacle->Destroy();
	if (!TestTrue(TEXT("The complete animal and poacher plan fits"), Director->BuildLayout(42, Player->GetActorLocation(), PoacherPlan)))
	{ AddError(Director->LastError); Cleanup(); return false; }
	TestEqual(TEXT("Animal total is unchanged"), Director->AnimalPlacements.Num(), 20);
	const TArray<FPPAnimalSpawnPlacement> First = Director->AnimalPlacements;
	TestTrue(TEXT("The same seed can be previewed again"), Director->BuildLayout(42, Player->GetActorLocation(), PoacherPlan));
	for (int32 I = 0; I < First.Num(); ++I)
		TestTrue(TEXT("Seeded animal transforms are repeatable"), First[I].Transform.Equals(Director->AnimalPlacements[I].Transform));
	Director->AnimalProfiles[0].MinGroupSize = 5; Director->AnimalProfiles[0].MaxGroupSize = 5;
	TestFalse(TEXT("An incompatible group size rejects the whole plan"), Director->BuildLayout(42, Player->GetActorLocation(), PoacherPlan));
	TestTrue(TEXT("A failed plan exposes no animal placements"), Director->AnimalPlacements.IsEmpty());
	TestTrue(TEXT("A failed plan exposes no poacher placements"), PoacherPlan.IsEmpty());
	for (int32 I = 0; I < Animals.Num(); ++I) TestTrue(TEXT("Preview failure never moves an animal"), Animals[I]->GetActorTransform().Equals(Originals[I]));
	Director->AnimalProfiles[0].MinGroupSize = 3; Director->AnimalProfiles[0].MaxGroupSize = 5;
	TArray<APPAnimalCharacter*> OrderedHerbivores, OrderedPredators;
	for (APPAnimalCharacter* A : Animals) (A->IsPredator() ? OrderedPredators : OrderedHerbivores).Add(A);
	OrderedHerbivores.Sort([](const APPAnimalCharacter& A, const APPAnimalCharacter& B) { return A.GetName() < B.GetName(); });
	OrderedPredators.Sort([](const APPAnimalCharacter& A, const APPAnimalCharacter& B) { return A.GetName() < B.GetName(); });
	FRandomStream FallbackRandom(42); TArray<int32> FallbackSizes;
	APPSpawnDirector::PartitionPopulation(13, 3, 5, FallbackRandom, FallbackSizes);
	FallbackSizes.Sort([](int32 A, int32 B) { return A > B; });
	int32 Offset = 0;
	for (int32 G = 0; G < FallbackSizes.Num(); ++G)
	{
		for (int32 M = 0; M < FallbackSizes[G]; ++M)
		{
			const float Angle = 2 * PI * M / FallbackSizes[G];
			OrderedHerbivores[Offset++]->SetActorLocation(FVector(18000 + G * 10000 + 1000 * FMath::Cos(Angle), 40000 + 1000 * FMath::Sin(Angle), 100));
		}
	}
	for (int32 I = 0; I < OrderedPredators.Num(); ++I) OrderedPredators[I]->SetActorLocation(FVector(12000 + I * 6500, 20000, 100));
	TestTrue(TEXT("A safe authored group layout is accepted as fallback"), Director->BuildAuthoredLayout(42, Player->GetActorLocation(), PoacherPlan));
	TestEqual(TEXT("Fallback preserves all twenty animals"), Director->AnimalPlacements.Num(), 20);
	for (const auto& P : Director->AnimalPlacements) TestTrue(TEXT("Fallback is explicitly marked"), P.bUsedFallback);
	for (int32 I = 0; I < Animals.Num(); ++I) Animals[I]->SetActorTransform(Originals[I]);
	UEnvironmentLevelSubsystem* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>(); Rules->OnWorldBeginPlay(*World);
	for (APPAnimalCharacter* A : Animals) A->DispatchBeginPlay();
	for (APPPoacherCharacter* P : Poachers) P->DispatchBeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	if (!TestTrue(TEXT("Population preparation succeeds"), Director->PreparePatrol(Player))) { AddError(Director->LastError); Cleanup(); return false; }
	const int32 HerdCount = Director->Herds.Num();
	TestTrue(TEXT("Preparation is idempotent"), Director->PreparePatrol(Player));
	TestEqual(TEXT("Repeated preparation creates no extra herds"), Director->Herds.Num(), HerdCount);
	TestEqual(TEXT("Herds divide thirteen elephants into groups of three to five"), HerdCount >= 3 && HerdCount <= 4, true);
	Rules->StartPatrol();
	TestEqual(TEXT("Round population remains twenty animals"), Rules->GetRoundSnapshot().TotalAnimals, 20);
	for (int32 I = 0; I < Animals.Num(); ++I)
	{
		TestEqual(TEXT("Only herbivores join herds"), Animals[I]->GetHerd() != nullptr, I < 13);
		if (I >= 13) { Animals[I]->StopAIUpdates(); Animals[I]->SetActorHiddenInGame(true); }
	}
	for (APPPoacherCharacter* P : Poachers) { P->StopAIUpdates(); P->SetActorHiddenInGame(true); }
	const FVector InitialPosition = Animals[0]->GetActorLocation();
	for (int32 I = 0; I < 160; ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestTrue(TEXT("A herd member navigated away from its spawn"), FVector::Dist2D(InitialPosition, Animals[0]->GetActorLocation()) > 100);
	APPAnimalHerd* Herd = Director->Herds[0];
	auto Members = Herd->GetLivingMembers();
	if (!TestTrue(TEXT("The herd has multiple living members"), Members.Num() >= 3)) { Cleanup(); return false; }
	APPAnimalCharacter* Observer = Members[0]; APPAnimalCharacter* Recipient = Members[1];
	// Place the reporting member near its peer without moving the recipient or inventing a live target.
	Observer->SetActorLocation(Recipient->GetActorLocation() + FVector(800, 0, 0));
	const FVector KnownThreat = Recipient->GetActorLocation() - FVector(1000, 0, 0);
	Herd->ReportThreat(Observer, KnownThreat);
	FVector Goal;
	TestFalse(TEXT("Danger suspends regrouping"), Herd->GetRoamGoal(Recipient, Goal));
	for (int32 I = 0; I < 5; ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestEqual(TEXT("A nearby member fled the reported danger"), Recipient->GetAnimalState(), EPPAnimalState::Fleeing);
	TestNull(TEXT("A shared alert contains no live target pointer"), Recipient->GetThreatActor());
	for (int32 I = 0; I < 150; ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestTrue(TEXT("Calm survivors can regroup again"), Herd->GetRoamGoal(Recipient, Goal));
	Observer->SetActorLocation(Recipient->GetActorLocation() + FVector(300, 0, 0));
	bool bCrowded = false;
	TestTrue(TEXT("Crowded members can choose a reachable separation goal"), Herd->GetSeparationGoal(Recipient, Goal, bCrowded));
	TestTrue(TEXT("The separation check detected crowding"), bCrowded);
	TestTrue(TEXT("The separation goal increases the gap"), FVector::Dist2D(Goal, Observer->GetActorLocation()) > 700);
	const int32 BeforeDeath = Herd->GetLivingMembers().Num();
	UGameplayStatics::ApplyDamage(Observer, 1000, nullptr, nullptr, nullptr);
	TestEqual(TEXT("A dead member leaves the herd"), Herd->GetLivingMembers().Num(), BeforeDeath - 1);
	TestEqual(TEXT("The loss is recorded once"), Rules->GetRoundSnapshot().AnimalsLost, 1);
	Rules->ReportAnimalLost(Observer, false);
	TestEqual(TEXT("Repeated loss reports stay idempotent"), Rules->GetRoundSnapshot().AnimalsLost, 1);
	TestEqual(TEXT("Death does not create replacement animals"), Rules->GetRoundSnapshot().TotalAnimals, 20);
	Cleanup();
	return true;
}

#endif
