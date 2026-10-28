#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Actors/PPSpawnDirector.h"
#include "Actors/PPArrestZone.h"
#include "Actors/PPTutorialDirector.h"
#include "Characters/PPPoacherCharacter.h"
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnvironmentLevelSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPSpawnAreaTest, "KrugerConclusion.PoachingPatrol.Spawning.PlayableArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPSpawnAreaTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APPSpawnDirector* Director = World->SpawnActor<APPSpawnDirector>();
	Director->SpawnBounds->SetBoxExtent(FVector(1000, 1000, 500));
	Director->PlayablePolygon = {{-900, -900}, {900, -900}, {900, 0}, {0, 0}, {0, 900}, {-900, 900}};
	TestTrue(TEXT("Inside the concave perimeter"), Director->IsInsidePlayableArea(FVector(-500, 500, 0), 100));
	TestFalse(TEXT("Concave cutout is excluded even inside the rectangular bounds"), Director->IsInsidePlayableArea(FVector(500, 500, 0)));
	TestFalse(TEXT("Boundary inset is respected"), Director->IsInsidePlayableArea(FVector(-850, 500, 0), 100));
	TestFalse(TEXT("Above the permitted height"), Director->IsInsidePlayableArea(FVector(-500, 500, 600)));
	Director->PlayablePolygon.Reset();
	Director->SetActorRotation(FRotator(0, 45, 0));
	TestTrue(TEXT("Rotated bounds accept their transformed interior"), Director->IsInsidePlayableArea(FVector(0, 1100, 0)));
	TestFalse(TEXT("Rotated bounds reject outside points"), Director->IsInsidePlayableArea(FVector(1100, 1100, 0)));
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPSpawnLayoutTest, "KrugerConclusion.PoachingPatrol.Spawning.LayoutAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPSpawnLayoutTest::RunTest(const FString& Parameters)
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
	Floor->SetActorLocation(FVector(10000, 10000, -50));
	Floor->SetActorScale3D(FVector(200, 200, 1));
	FloorMesh->RegisterComponent();
	ANavMeshBoundsVolume* Bounds = World->SpawnActor<ANavMeshBoundsVolume>(
		ANavMeshBoundsVolume::StaticClass(), FVector(10000, 10000, 0), FRotator::ZeroRotator);
	UCubeBuilder* Builder = NewObject<UCubeBuilder>();
	Builder->X = 20000; Builder->Y = 20000; Builder->Z = 2000;
	UActorFactory::CreateBrushForVolumeActor(Bounds, Builder);
	FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::GameMode);
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(World);
	if (!TestNotNull(TEXT("Navigation exists"), Navigation)) { Cleanup(); return false; }
	Navigation->OnNavigationBoundsUpdated(Bounds);
	Navigation->Tick(0.1f);
	ANavigationData* NavData = Navigation->GetDefaultNavDataInstance(FNavigationSystem::Create);
	if (!TestNotNull(TEXT("Navigation data exists"), NavData)) { Cleanup(); return false; }
	FEnumProperty* Generation = FindFProperty<FEnumProperty>(ANavigationData::StaticClass(), TEXT("RuntimeGeneration"));
	Generation->GetUnderlyingProperty()->SetIntPropertyValue(Generation->ContainerPtrToValuePtr<void>(NavData), static_cast<int64>(ERuntimeGenerationType::Dynamic));
	NavData->ConditionalConstructGenerator();
	Navigation->Build();
	NavData->EnsureBuildCompletion();
	APPSpawnDirector* Director = World->SpawnActor<APPSpawnDirector>();
	Director->SetActorLocation(FVector(10000, 10000, 0));
	Director->SpawnBounds->SetBoxExtent(FVector(9000, 9000, 900));
	Director->FixedSeed = 42;
	APlayerStart* Player = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(2000, 2000, 100), FRotator::ZeroRotator);
	Player->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	APPArrestZone* Camp = World->SpawnActor<APPArrestZone>(APPArrestZone::StaticClass(), FVector(2500, 2000, 100), FRotator::ZeroRotator);
	TArray<APPPoacherCharacter*> Roster;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 I = 0; I < 4; ++I)
	{
		Roster.Add(World->SpawnActor<APPPoacherCharacter>(APPPoacherCharacter::StaticClass(), FVector(5000 + 3000 * I, 5000, 100), FRotator::ZeroRotator, Spawn));
	}
	APPPoacherCharacter* Tutorial = World->SpawnActor<APPPoacherCharacter>(APPPoacherCharacter::StaticClass(), FVector(19000, 19000, 100), FRotator::ZeroRotator, Spawn);
	Tutorial->bTutorialEncounter = true;
	const FVector TutorialStart = Tutorial->GetActorLocation();
	TArray<FPPSpawnPlacement> First, Repeated, Different;
	if (!TestTrue(*FString::Printf(TEXT("Layout found: %s"), *Director->LastError), Director->BuildLayout(42, Player->GetActorLocation(), First))) { Cleanup(); return false; }
	TestEqual(TEXT("The authored roster is preserved"), First.Num(), 4);
	TestTrue(TEXT("Same seed can be generated twice"), Director->BuildLayout(42, Player->GetActorLocation(), Repeated));
	TestTrue(TEXT("A second seed can be generated"), Director->BuildLayout(17, Player->GetActorLocation(), Different));
	if (First.Num() != Repeated.Num() || First.Num() != Different.Num()) { Cleanup(); return false; }
	bool bChanged = false;
	for (int32 I = 0; I < First.Num(); ++I)
	{
		TestTrue(TEXT("Same seed yields the same transforms"), First[I].Transform.Equals(Repeated[I].Transform));
		bChanged |= !First[I].Transform.Equals(Different[I].Transform);
		TestFalse(TEXT("Tutorial poacher is excluded"), First[I].Poacher == Tutorial);
		for (int32 J = 0; J < I; ++J)
			TestTrue(TEXT("Minimum poacher spacing holds"), FVector::Dist2D(First[I].Transform.GetLocation(), First[J].Transform.GetLocation()) >= Director->PoacherSpacing);
	}
	TestTrue(TEXT("A new seed changes the layout"), bChanged);
	Director->AttemptsPerRegion = 1;
	Director->PlayablePolygon = {{4000, 4500}, {16000, 4500}, {16000, 5500}, {4000, 5500}};
	TArray<FPPSpawnPlacement> Fallback;
	TestTrue(TEXT("Validated authored positions provide bounded fallback"), Director->BuildLayout(42, Player->GetActorLocation(), Fallback));
	TestTrue(TEXT("The narrow area exercised fallback placement"), Fallback.ContainsByPredicate([](const FPPSpawnPlacement& P) { return P.bUsedFallback; }));
	Director->PlayablePolygon.Reset();
	Director->AttemptsPerRegion = 100;
	AActor* Exclusion = World->SpawnActor<AActor>();
	UBoxComponent* ExclusionBox = NewObject<UBoxComponent>(Exclusion);
	Exclusion->SetRootComponent(ExclusionBox);
	ExclusionBox->SetBoxExtent(FVector(10000, 10000, 1000));
	ExclusionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExclusionBox->RegisterComponent();
	Exclusion->SetActorLocation(FVector(10000, 10000, 0));
	Director->ExclusionAreas.Add(Exclusion);
	TestFalse(TEXT("Exclusion volumes reject both random and fallback placements"), Director->BuildLayout(42, Player->GetActorLocation(), Fallback));
	TestTrue(TEXT("A failed layout exposes no partial roster"), Fallback.IsEmpty());
	Director->ExclusionAreas.Reset();
	// Blueprint capsules can have a Custom profile with no named preset. Test their actual responses.
	for (APPPoacherCharacter* P : Roster) P->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	ExclusionBox->SetCanEverAffectNavigation(false);
	ExclusionBox->SetCollisionObjectType(ECC_WorldDynamic);
	ExclusionBox->SetCollisionResponseToAllChannels(ECR_Block);
	ExclusionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TestFalse(TEXT("Dynamic obstacles block custom-profile capsules even over valid navigation"), Director->BuildLayout(42, Player->GetActorLocation(), Fallback));
	TestTrue(TEXT("Capsule clearance rejected the obstacle"), Director->Rejections.FindRef(TEXT("BlockedCapsule")) > 0);
	ExclusionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UEnvironmentLevelSubsystem* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	Rules->OnWorldBeginPlay(*World);
	TestTrue(TEXT("AI waits for complete placement"), Rules->IsPatrolSetupPending());
	for (APPPoacherCharacter* P : Roster) P->DispatchBeginPlay();
	Camp->DispatchBeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	TestTrue(TEXT("Patrol preparation succeeds"), Director->PreparePatrol(Player));
	TestTrue(TEXT("A second call reuses the completed population"), Director->PreparePatrol(Player));
	Rules->StartPatrol();
	TestTrue(TEXT("The clock can start after preparation"), Rules->HasPatrolStarted());
	TestEqual(TEXT("Round count did not double"), Rules->GetRoundSnapshot().TotalPoachers, 4);
	FStructProperty* Home = FindFProperty<FStructProperty>(APPCreatureBase::StaticClass(), TEXT("HomeLocation"));
	for (const FPPSpawnPlacement& P : Director->Placements)
	{
		TestTrue(TEXT("Home moved with the poacher"), Home->ContainerPtrToValuePtr<FVector>(P.Poacher.Get())->Equals(P.Transform.GetLocation()));
		TestNotNull(TEXT("Spawned poacher has an AI controller"), P.Poacher->GetController());
	}
	const FVector BeforeMove = Roster[0]->GetActorLocation();
	for (int32 I = 0; I < 40; ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All, 0.1f); }
	TestTrue(TEXT("Poacher pathfinding moved away from its new spawn"), FVector::Dist2D(BeforeMove, Roster[0]->GetActorLocation()) > 20);
	TestTrue(TEXT("Tutorial position is untouched"), Tutorial->GetActorLocation().Equals(TutorialStart));
	Player->SetActorLocation(Roster[0]->GetActorLocation() + FVector(250, 0, 0));
	Roster[0]->CapturePoacher(Player);
	TestTrue(TEXT("Relocated poacher remains capturable"), Roster[0]->IsCaptured());
	const FVector EscortGoal = Camp->GetActorLocation() + FVector(200, 0, 0);
	// Steer the captor across the test floor; the captive uses actual navigation and the arrest overlap.
	for (int32 I = 0; I < 1800 && Roster[0]->GetPoacherState() != EPPPoacherState::Arrested; ++I)
	{
		if (FVector::Dist2D(Player->GetActorLocation(), Roster[0]->GetActorLocation()) < 550)
			Player->SetActorLocation(FMath::VInterpConstantTo(Player->GetActorLocation(), EscortGoal, 0.1f, 300));
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.1f);
	}
	TestEqual(TEXT("Captive navigated into the arrest zone"), Roster[0]->GetPoacherState(), EPPPoacherState::Arrested);
	Rules->ReportPoacherArrested(Roster[0]);
	TestEqual(TEXT("Arrest is recorded once"), Rules->GetRoundSnapshot().PoachersArrested, 1);
	Cleanup();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPPSpawnFailureTest, "KrugerConclusion.PoachingPatrol.Spawning.FailureAndTutorial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPSpawnFailureTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APPSpawnDirector* Director = World->SpawnActor<APPSpawnDirector>();
	Director->AttemptsPerRegion = 1;
	APPPoacherCharacter* Poacher = World->SpawnActor<APPPoacherCharacter>();
	APlayerStart* Player = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(1000, 1000, 100), FRotator::ZeroRotator);
	UEnvironmentLevelSubsystem* Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	Rules->OnWorldBeginPlay(*World);
	const FTransform Original = Poacher->GetActorTransform();
	AddExpectedError(TEXT("Patrol spawn setup failed"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("No navigation fails preparation"), Director->PreparePatrol(Player));
	TestFalse(TEXT("Failure is not retried every player tick"), Director->PreparePatrol(Player));
	Rules->StartPatrol();
	TestFalse(TEXT("Failed setup leaves the clock stopped"), Rules->HasPatrolStarted());
	TestTrue(TEXT("Failed setup preserves original transforms"), Poacher->GetActorTransform().Equals(Original));
	TestEqual(TEXT("Failed setup preserves round population"), Rules->GetRoundSnapshot().TotalPoachers, 1);
	TestEqual(TEXT("The player receives setup failure guidance"), Rules->GetObjectivesForPlayer(Player)[0].Identifier, FName(TEXT("PatrolSetup")));
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APPTutorialDirector* Tutorial = World->SpawnActor<APPTutorialDirector>();
	Tutorial->PatrolMode = EPPPatrolMode::Tutorial;
	APPSpawnDirector* TutorialSpawn = World->SpawnActor<APPSpawnDirector>();
	Poacher = World->SpawnActor<APPPoacherCharacter>();
	Player = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(1000, 1000, 100), FRotator::ZeroRotator);
	Rules = World->GetSubsystem<UEnvironmentLevelSubsystem>();
	Rules->OnWorldBeginPlay(*World);
	TestFalse(TEXT("Tutorial never prepares procedural encounters"), TutorialSpawn->PreparePatrol(Player));
	TestTrue(TEXT("Tutorial poacher position stays authored"), Poacher->GetActorTransform().Equals(Original));
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
