#include "Actors/PPSpawnDirector.h"

#include "Actors/PPArrestZone.h"
#include "Actors/PPAnimalHerd.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/ARangerCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "EnvironmentLevelSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationData.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

namespace
{
bool HasCompletePath(UNavigationSystemV1* Navigation, const ACharacter* Agent, const FVector& Start,
	const FVector& End, float MaxLength)
{
	const FNavAgentProperties& Props = Agent->GetNavAgentPropertiesRef();
	const ANavigationData* Data = Navigation->GetNavDataForProps(Props, Start);
	if (!Data) return false;
	FPathFindingQuery Query(Agent, *Data, Start, End);
	Query.SetAllowPartialPaths(false).SetNavAgentProperties(Props);
	const FPathFindingResult Result = Navigation->FindPathSync(Props, Query);
	return Result.IsSuccessful() && Result.Path.IsValid() && !Result.Path->IsPartial()
		&& Result.Path->GetLength() <= MaxLength;
}

FVector GetPreviewPlayerLocation(UWorld* World)
{
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("PPTutorial"))) return It->GetActorLocation();
	}
	return FVector::ZeroVector;
}
}

APPSpawnDirector::APPSpawnDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBounds"));
	SetRootComponent(SpawnBounds);
	SpawnBounds->SetBoxExtent(FVector(20000, 20000, 5000));
	SpawnBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBounds->SetCanEverAffectNavigation(false);
	PopulationStreamingSource = CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("PopulationStreamingSource"));
	PopulationStreamingSource->DisableStreamingSource();
	FPPAnimalSpawnProfile Elephants;
	Elephants.Species = FGameplayTag::RequestGameplayTag(TEXT("Elephant"), false);
	AnimalProfiles.Add(Elephants);
	FPPAnimalSpawnProfile Lions;
	Lions.Species = FGameplayTag::RequestGameplayTag(TEXT("Lion"), false);
	Lions.bPredator = true; Lions.bHerding = false;
	Lions.MinGroupSize = Lions.MaxGroupSize = 1;
	Lions.PlayerSafeDistance = 8000; Lions.CampSafeDistance = 6000; Lions.GroupSpacing = 4000;
	AnimalProfiles.Add(Lions);
	FPPAnimalSpawnProfile Zebras;
	Zebras.Species = FGameplayTag::RequestGameplayTag(TEXT("Zebra"), false);
	Zebras.SpreadRadius = 1500; Zebras.MemberSpacing = 500; Zebras.GroupSpacing = 4500;
	Zebras.RoamRadius = 3500; Zebras.RegroupDistance = 2500;
	AnimalProfiles.Add(Zebras);
	FPPAnimalSpawnProfile Rhinos;
	Rhinos.Species = FGameplayTag::RequestGameplayTag(TEXT("Rhino"), false);
	Rhinos.bHerding = false; Rhinos.MinGroupSize = Rhinos.MaxGroupSize = 1;
	Rhinos.SpreadRadius = 1000; Rhinos.MemberSpacing = 650; Rhinos.GroupSpacing = 4500;
	Rhinos.RoamRadius = 3500; Rhinos.RegroupDistance = 2500;
	AnimalProfiles.Add(Rhinos);
#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

bool APPSpawnDirector::IsInsidePlayableArea(const FVector& Location, float Inset) const
{
	const FVector Local = SpawnBounds->GetComponentTransform().InverseTransformPosition(Location);
	const FVector Extent = SpawnBounds->GetUnscaledBoxExtent();
	if (FMath::Abs(Local.X) > Extent.X - Inset || FMath::Abs(Local.Y) > Extent.Y - Inset
		|| FMath::Abs(Local.Z) > Extent.Z) return false;
	if (PlayablePolygon.IsEmpty()) return true;
	if (PlayablePolygon.Num() < 3) return false;
	const FVector2D Point(Location.X, Location.Y);
	bool bInside = false;
	for (int32 I = 0, J = PlayablePolygon.Num() - 1; I < PlayablePolygon.Num(); J = I++)
	{
		const FVector2D A = PlayablePolygon[J];
		const FVector2D B = PlayablePolygon[I];
		const FVector2D Edge = B - A;
		const double T = FMath::Clamp(FVector2D::DotProduct(Point - A, Edge) / FMath::Max(Edge.SizeSquared(), 0.001), 0.0, 1.0);
		if (FVector2D::DistSquared(Point, A + T * Edge) <= FMath::Square(Inset)) return false;
		if ((A.Y > Point.Y) != (B.Y > Point.Y)
			&& Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X) bInside = !bInside;
	}
	return bInside;
}

bool APPSpawnDirector::ValidateCandidate(APPPoacherCharacter* Poacher, const FVector& RawLocation,
	const FVector& PlayerLocation, const TArray<APPPoacherCharacter*>& Roster,
	const TArray<FPPSpawnPlacement>& Planned, FVector& OutLocation, FName& Reason) const
{
	if (!ProjectCreatureLocation(Poacher, RawLocation, OutLocation, Reason)) return false;
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	const FNavAgentProperties& Props = Poacher->GetNavAgentPropertiesRef();
	const FVector NavPoint = OutLocation - FVector(0, 0, Poacher->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	Reason = TEXT("NearPlayer");
	if (FVector::DistSquared2D(OutLocation, PlayerLocation) < FMath::Square(PlayerSafeDistance)) return false;
	Reason = TEXT("NearAnimal");
	for (TActorIterator<APPAnimalCharacter> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("PPTutorial"))) continue;
        FVector AnimalLocation = It->GetActorLocation();
        if (const FPPAnimalSpawnPlacement* Placement = AnimalPlacements.FindByPredicate([&](const FPPAnimalSpawnPlacement& P) { return P.Animal == *It; })) AnimalLocation = Placement->Transform.GetLocation();
        const float Separation = FMath::Max(AnimalSafeDistance, GetBodyRadius(Poacher) + GetBodyRadius(*It) + 100);
        if (FVector::DistSquared2D(OutLocation, AnimalLocation) < FMath::Square(Separation)) return false;
	}
	Reason = TEXT("NearPoacher");
	for (const FPPSpawnPlacement& Other : Planned)
	{
		if (FVector::DistSquared2D(OutLocation, Other.Transform.GetLocation()) < FMath::Square(PoacherSpacing)) return false;
	}
	// Reserve the still-unassigned authored positions so they remain available as validated fallbacks.
	for (APPPoacherCharacter* Other : Roster)
	{
		if (Other != Poacher && !Planned.ContainsByPredicate([Other](const FPPSpawnPlacement& P) { return P.Poacher == Other; })
			&& FVector::DistSquared2D(OutLocation, Other->GetActorLocation()) < FMath::Square(PoacherSpacing)) return false;
	}
	Reason = TEXT("NoEscortRoute");
	bool bHasCampRoute = false;
	for (TActorIterator<APPArrestZone> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("PPTutorial"))) continue;
		if (FVector::DistSquared2D(OutLocation, It->GetActorLocation()) < FMath::Square(CampSafeDistance)) { Reason = TEXT("NearCamp"); return false; }
		FNavLocation Camp;
		if (!Navigation->ProjectPointToNavigation(It->GetActorLocation(), Camp, FVector(200, 200, 500), &Props)) continue;
		if (HasCompletePath(Navigation, Poacher, NavPoint, Camp.Location, MaxEscortPathLength)
			&& HasCompletePath(Navigation, Poacher, Camp.Location, NavPoint, MaxEscortPathLength)) bHasCampRoute = true;
	}
	if (!bHasCampRoute) return false;
	Reason = TEXT("NoPlayerRoute");
	const ACharacter* PlayerAgent = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerAgent) PlayerAgent = GetDefault<ARangerCharacter>();
	FNavLocation PlayerNav;
	if (!Navigation->ProjectPointToNavigation(PlayerLocation, PlayerNav, FVector(200, 200, 500), &PlayerAgent->GetNavAgentPropertiesRef())
		|| !HasCompletePath(Navigation, PlayerAgent, PlayerNav.Location, NavPoint, MaxEscortPathLength)) return false;
	Reason = TEXT("NoRoamingSpace");
	int32 ReachableProbes = 0;
	for (const FVector& Offset : {FVector(RoamProbeDistance, 0, 0), FVector(-RoamProbeDistance, 0, 0),
		FVector(0, RoamProbeDistance, 0), FVector(0, -RoamProbeDistance, 0)})
	{
		FNavLocation Probe;
		if (Navigation->ProjectPointToNavigation(NavPoint + Offset, Probe, FVector(75, 75, 200), &Props)
			&& IsInsidePlayableArea(Probe.Location, BoundaryInset)
			&& HasCompletePath(Navigation, Poacher, NavPoint, Probe.Location, RoamProbeDistance * 3)) ++ReachableProbes;
	}
	return ReachableProbes >= 2;
}

void APPSpawnDirector::RecordRejection(const FVector& Location, FName Reason)
{
	++Rejections.FindOrAdd(Reason);
	if (bDrawSpawnDebug) DrawDebugPoint(GetWorld(), Location, 8, FColor::Red, false, 15.0f);
}

bool APPSpawnDirector::BuildPoacherLayout(int32 Seed, const FVector& PlayerLocation, TArray<FPPSpawnPlacement>& OutPlacements)
{
	OutPlacements.Reset();
	TArray<APPPoacherCharacter*> Roster;
	for (TActorIterator<APPPoacherCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->bTutorialEncounter && !It->bStartEncounterInactive) Roster.Add(*It);
	}
	// World Partition iteration order can change. Seeded placement uses a stable actor order.
	Roster.Sort([](const APPPoacherCharacter& A, const APPPoacherCharacter& B) { return A.GetName() < B.GetName(); });
	if (Roster.IsEmpty()) { LastError = TEXT("No normal patrol poachers are available."); return false; }
	FRandomStream Random(Seed);
	TArray<int32> RegionOrder = {0, 1, 2, 3};
	for (int32 I = 3; I > 0; --I) RegionOrder.Swap(I, Random.RandRange(0, I));
	int32 RegionPopulation[4] = {0, 0, 0, 0};
	const FVector Extent = SpawnBounds->GetUnscaledBoxExtent();
	for (APPPoacherCharacter* Poacher : Roster)
	{
		TArray<int32> Regions = RegionOrder;
		Regions.StableSort([&](int32 A, int32 B) { return RegionPopulation[A] < RegionPopulation[B]; });
		bool bPlaced = false;
		for (int32 Region : Regions)
		{
			for (int32 Attempt = 0; Attempt < FMath::Clamp(AttemptsPerRegion, 1, 500); ++Attempt)
			{
				const FVector Local(Random.FRandRange((Region & 1) ? 0 : -Extent.X, (Region & 1) ? Extent.X : 0),
					Random.FRandRange((Region & 2) ? 0 : -Extent.Y, (Region & 2) ? Extent.Y : 0), 0);
				const FVector Raw = SpawnBounds->GetComponentTransform().TransformPosition(Local);
				FVector Location;
				FName Reason;
				if (!ValidateCandidate(Poacher, Raw, PlayerLocation, Roster, OutPlacements, Location, Reason))
				{
					RecordRejection(Raw, Reason);
					continue;
				}
				FPPSpawnPlacement Placement;
				Placement.Poacher = Poacher;
				Placement.Transform = FTransform(FRotator(0, Random.FRandRange(-180, 180), 0), Location, Poacher->GetActorScale3D());
				OutPlacements.Add(Placement);
				++RegionPopulation[Region];
				bPlaced = true;
				break;
			}
			if (bPlaced) break;
		}
		if (!bPlaced)
		{
			FVector Location;
			FName Reason;
			if (!ValidateCandidate(Poacher, Poacher->GetActorLocation(), PlayerLocation, Roster, OutPlacements, Location, Reason))
			{
				RecordRejection(Poacher->GetActorLocation(), Reason);
				LastError = FString::Printf(TEXT("No safe position for %s; fallback rejected: %s."), *Poacher->GetName(), *Reason.ToString());
				OutPlacements.Reset();
				return false;
			}
			FPPSpawnPlacement Placement;
			Placement.Poacher = Poacher;
			Placement.Transform = FTransform(Poacher->GetActorRotation(), Location, Poacher->GetActorScale3D());
			Placement.bUsedFallback = true;
			OutPlacements.Add(Placement);
		}
	}
	return true;
}

bool APPSpawnDirector::BuildLayout(int32 Seed, const FVector& PlayerLocation, TArray<FPPSpawnPlacement>& OutPlacements)
{
	OutPlacements.Reset(); Rejections.Reset(); LastError.Reset();
	bool bHasAnimals = false;
	if (bRandomiseAnimals)
		for (TActorIterator<APPAnimalCharacter> It(GetWorld()); It; ++It)
			if (!It->ActorHasTag(TEXT("PPTutorial"))) { bHasAnimals = true; break; }
	if (!bHasAnimals)
	{
		AnimalPlacements.Reset(); AnimalGroups.Reset();
		return BuildPoacherLayout(Seed, PlayerLocation, OutPlacements);
	}
	for (int32 Attempt = 0; Attempt < FMath::Clamp(LayoutAttempts, 1, 8); ++Attempt)
	{
		LastError.Reset();
		const uint32 AttemptSeed = HashCombineFast(GetTypeHash(Seed), GetTypeHash(Attempt));
		if (BuildAnimalLayout(static_cast<int32>(HashCombineFast(AttemptSeed, 0xA11A1)), PlayerLocation)
			&& BuildPoacherLayout(static_cast<int32>(HashCombineFast(AttemptSeed, 0xB0AC4)), PlayerLocation, OutPlacements)) return true;
		UE_LOG(LogTemp, Verbose, TEXT("Patrol layout attempt %d seed=%d: %s"), Attempt + 1, Seed, *LastError);
	}
	return BuildAuthoredLayout(Seed, PlayerLocation, OutPlacements);
}

bool APPSpawnDirector::BuildAuthoredLayout(int32 Seed, const FVector& PlayerLocation, TArray<FPPSpawnPlacement>& OutPlacements)
{
	LastError.Reset();
	if (BuildAnimalLayout(Seed, PlayerLocation, true) && BuildPoacherLayout(Seed, PlayerLocation, OutPlacements)) return true;
	OutPlacements.Reset(); AnimalPlacements.Reset(); AnimalGroups.Reset();
	return false;
}

bool APPSpawnDirector::PreviewLayout(int32 Seed)
{
	if (GetWorld()->IsGameWorld()) return false;
	ActiveSeed = Seed;
	const bool bSuccess = BuildLayout(Seed, GetPreviewPlayerLocation(GetWorld()), Placements);
	if (bDrawSpawnDebug)
	{
		for (const FPPSpawnPlacement& P : Placements) DrawDebugSphere(GetWorld(), P.Transform.GetLocation(), 100, 12, FColor::Green, false, 30);
		for (const FPPAnimalSpawnPlacement& P : AnimalPlacements) DrawDebugSphere(GetWorld(), P.Transform.GetLocation(), GetBodyRadius(P.Animal), 12, FColor::Cyan, false, 30);
	}
	return bSuccess;
}

bool APPSpawnDirector::PreparePatrol(AActor* Player)
{
	if (bPreparationAttempted) return bPrepared;
	const UEnvironmentLevelSubsystem* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>();
	if (!Player || (Rules && Rules->IsTutorialMode())) return false;
	if (GetWorld()->GetGameInstance())
	{
		const auto* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
		UWorldPartitionSubsystem* Partition = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>();
		if (Partition && GetWorld()->GetWorldPartition() && !bStreamingRequested)
		{
			FStreamingSourceShape Shape;
			Shape.bUseGridLoadingRange = false;
			Shape.Radius = SpawnBounds->GetScaledBoxExtent().Size2D();
			PopulationStreamingSource->Shapes = {Shape};
			PopulationStreamingSource->EnableStreamingSource();
			bStreamingRequested = bWaitingForWorld = true;
			return false;
		}
		bWaitingForWorld = GetWorld()->GetTimeSeconds() < 0.5f || !Navigation || !Navigation->GetDefaultNavDataInstance()
			|| UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(GetWorld()) || (Partition && !Partition->IsAllStreamingCompleted())
			|| (bStreamingRequested && !PopulationStreamingSource->IsStreamingCompleted());
		if (bWaitingForWorld) return false;
	}
	bWaitingForWorld = false;
	bPreparationAttempted = true;
	ActiveSeed = FixedSeed != 0 ? FixedSeed : static_cast<int32>(FPlatformTime::Cycles());
	if (!BuildLayout(ActiveSeed, Player->GetActorLocation(), Placements))
	{
		UE_LOG(LogTemp, Error, TEXT("Patrol spawn setup failed (seed %d): %s"), ActiveSeed, *LastError);
		for (const auto& Rejection : Rejections) UE_LOG(LogTemp, Display, TEXT("Spawn rejection %s: %d"), *Rejection.Key.ToString(), Rejection.Value);
		return false;
	}
	// Validate every destination before moving anyone. Keep per-instance Blueprint configuration and roster identity.
	TMap<int32, APPAnimalHerd*> PreparedHerds;
	for (int32 GroupId = 0; GroupId < AnimalGroups.Num(); ++GroupId)
	{
		const auto& Profile = AnimalProfiles[AnimalGroups[GroupId].ProfileIndex];
		if (!Profile.bHerding || Profile.bPredator) continue;
		APPAnimalHerd* Herd = GetWorld()->SpawnActor<APPAnimalHerd>();
		if (!Herd)
		{
			for (const auto& Prepared : PreparedHerds) Prepared.Value->Destroy();
			LastError = TEXT("Could not initialise animal herds.");
			UE_LOG(LogTemp, Error, TEXT("Patrol spawn setup failed: %s"), *LastError);
			return false;
		}
		PreparedHerds.Add(GroupId, Herd);
	}
	for (const FPPAnimalSpawnPlacement& P : AnimalPlacements)
	{
		P.Animal->StopAIUpdates();
		P.Animal->StopMovement();
		P.Animal->SetActorLocationAndRotation(P.Transform.GetLocation(), P.Transform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		P.Animal->ResetRoamingHome();
		P.Animal->StartIdle();
	}
	for (int32 GroupId = 0; GroupId < AnimalGroups.Num(); ++GroupId)
	{
		const FPPAnimalGroupPlan& Group = AnimalGroups[GroupId];
		const FPPAnimalSpawnProfile& Profile = AnimalProfiles[Group.ProfileIndex];
		if (!Profile.bHerding || Profile.bPredator) continue;
		APPAnimalHerd* Herd = PreparedHerds[GroupId];
		TArray<APPAnimalCharacter*> Members;
		for (const FPPAnimalSpawnPlacement& P : AnimalPlacements) if (P.GroupId == GroupId) Members.Add(P.Animal);
		Herd->Initialise(this, Group.Center, Profile, Members, HashCombineFast(GetTypeHash(ActiveSeed), GroupId));
		Herds.Add(Herd);
	}
	for (const FPPSpawnPlacement& P : Placements)
	{
		P.Poacher->SetEncounterActive(false);
		P.Poacher->SetActorLocationAndRotation(P.Transform.GetLocation(), P.Transform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		P.Poacher->ResetRoamingHome();
	}
	for (const FPPSpawnPlacement& P : Placements)
	{
		P.Poacher->SetEncounterActive(true);
		P.Poacher->StartDisguisedRoaming();
		if (bDrawSpawnDebug) DrawDebugCapsule(GetWorld(), P.Transform.GetLocation(), P.Poacher->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
			P.Poacher->GetCapsuleComponent()->GetScaledCapsuleRadius(), FQuat::Identity, FColor::Green, false, 30);
		UE_LOG(LogTemp, Display, TEXT("Patrol spawn seed=%d poacher=%s position=%s fallback=%d"), ActiveSeed,
			*P.Poacher->GetName(), *P.Transform.GetLocation().ToCompactString(), P.bUsedFallback);
	}
	bPrepared = true;
	for (const FPPAnimalSpawnPlacement& P : AnimalPlacements) P.Animal->StartAIUpdates();
	UE_LOG(LogTemp, Display, TEXT("Patrol animal seed=%d animals=%d herds=%d"), ActiveSeed, AnimalPlacements.Num(), Herds.Num());
	return true;
}
