#include "Actors/PPSpawnDirector.h"

#include "Actors/PPArrestZone.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/ARangerCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "EnvironmentLevelSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationData.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

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
	Reason = TEXT("OutsideArea");
	if (!IsInsidePlayableArea(RawLocation, BoundaryInset)) return false;
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Navigation) { Reason = TEXT("NoNavigation"); return false; }
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PPSpawnGround), false);
	for (APPPoacherCharacter* Existing : Roster) TraceParams.AddIgnoredActor(Existing);
	const FBox Bounds = SpawnBounds->Bounds.GetBox();
	FHitResult Ground;
	const FVector Top(RawLocation.X, RawLocation.Y, Bounds.Max.Z);
	const FVector Bottom(RawLocation.X, RawLocation.Y, Bounds.Min.Z);
	Reason = TEXT("UnsafeGround");
	if (!GetWorld()->LineTraceSingleByObjectType(Ground, Top, Bottom,
		FCollisionObjectQueryParams(ECC_WorldStatic), TraceParams)
		|| !Poacher->GetCharacterMovement()->IsWalkable(Ground)) return false;
	if (Ground.GetActor() && (Ground.GetActor()->ActorHasTag(TEXT("PPNoSpawn"))
		|| Ground.GetActor()->ActorHasTag(TEXT("Water")))) return false;
	FNavLocation NavLocation;
	const FNavAgentProperties& Props = Poacher->GetNavAgentPropertiesRef();
	Reason = TEXT("OffNavigation");
	if (!Navigation->ProjectPointToNavigation(Ground.ImpactPoint, NavLocation, FVector(100, 100, 200), &Props)) return false;
	if (!GetWorld()->LineTraceSingleByObjectType(Ground, NavLocation.Location + FVector(0, 0, 200),
		NavLocation.Location - FVector(0, 0, 250), FCollisionObjectQueryParams(ECC_WorldStatic), TraceParams)
		|| !Poacher->GetCharacterMovement()->IsWalkable(Ground)) return false;
	if (FMath::Abs(Ground.ImpactPoint.Z - NavLocation.Location.Z) > FMath::Max(50.0f, Poacher->GetCharacterMovement()->MaxStepHeight)
		|| (Ground.GetActor() && (Ground.GetActor()->ActorHasTag(TEXT("PPNoSpawn")) || Ground.GetActor()->ActorHasTag(TEXT("Water"))))) return false;
	const UCapsuleComponent* Capsule = Poacher->GetCapsuleComponent();
	OutLocation = Ground.ImpactPoint + FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight() + 3.0f);
	Reason = TEXT("OutsideArea");
	if (!IsInsidePlayableArea(OutLocation, FMath::Max(BoundaryInset, Capsule->GetScaledCapsuleRadius()))) return false;
	Reason = TEXT("ExcludedArea");
	for (const AActor* Area : ExclusionAreas)
	{
		if (IsValid(Area) && Area->GetComponentsBoundingBox(true).ExpandBy(FVector(Capsule->GetScaledCapsuleRadius(),
			Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight())).IsInsideOrOn(OutLocation)) return false;
	}
	Reason = TEXT("NearPlayer");
	if (FVector::DistSquared2D(OutLocation, PlayerLocation) < FMath::Square(PlayerSafeDistance)) return false;
	Reason = TEXT("NearAnimal");
	for (TActorIterator<APPAnimalCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("PPTutorial")) && FVector::DistSquared2D(OutLocation, It->GetActorLocation()) < FMath::Square(AnimalSafeDistance)) return false;
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
	Reason = TEXT("BlockedCapsule");
	if (GetWorld()->OverlapBlockingTestByChannel(OutLocation, FQuat::Identity, Capsule->GetCollisionObjectType(),
		FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), TraceParams,
		FCollisionResponseParams(Capsule->GetCollisionResponseToChannels()))) return false;
	Reason = TEXT("NoEscortRoute");
	bool bHasCampRoute = false;
	for (TActorIterator<APPArrestZone> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("PPTutorial"))) continue;
		if (FVector::DistSquared2D(OutLocation, It->GetActorLocation()) < FMath::Square(CampSafeDistance)) { Reason = TEXT("NearCamp"); return false; }
		FNavLocation Camp;
		if (!Navigation->ProjectPointToNavigation(It->GetActorLocation(), Camp, FVector(200, 200, 500), &Props)) continue;
		if (HasCompletePath(Navigation, Poacher, NavLocation.Location, Camp.Location, MaxEscortPathLength)
			&& HasCompletePath(Navigation, Poacher, Camp.Location, NavLocation.Location, MaxEscortPathLength)) bHasCampRoute = true;
	}
	if (!bHasCampRoute) return false;
	Reason = TEXT("NoPlayerRoute");
	const ACharacter* PlayerAgent = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerAgent) PlayerAgent = GetDefault<ARangerCharacter>();
	FNavLocation PlayerNav;
	if (!Navigation->ProjectPointToNavigation(PlayerLocation, PlayerNav, FVector(200, 200, 500), &PlayerAgent->GetNavAgentPropertiesRef())
		|| !HasCompletePath(Navigation, PlayerAgent, PlayerNav.Location, NavLocation.Location, MaxEscortPathLength)) return false;
	Reason = TEXT("NoRoamingSpace");
	int32 ReachableProbes = 0;
	for (const FVector& Offset : {FVector(RoamProbeDistance, 0, 0), FVector(-RoamProbeDistance, 0, 0),
		FVector(0, RoamProbeDistance, 0), FVector(0, -RoamProbeDistance, 0)})
	{
		FNavLocation Probe;
		if (Navigation->ProjectPointToNavigation(NavLocation.Location + Offset, Probe, FVector(75, 75, 200), &Props)
			&& IsInsidePlayableArea(Probe.Location, BoundaryInset)
			&& HasCompletePath(Navigation, Poacher, NavLocation.Location, Probe.Location, RoamProbeDistance * 3)) ++ReachableProbes;
	}
	return ReachableProbes >= 2;
}

void APPSpawnDirector::RecordRejection(const FVector& Location, FName Reason)
{
	++Rejections.FindOrAdd(Reason);
	if (bDrawSpawnDebug) DrawDebugPoint(GetWorld(), Location, 8, FColor::Red, false, 15.0f);
}

bool APPSpawnDirector::BuildLayout(int32 Seed, const FVector& PlayerLocation, TArray<FPPSpawnPlacement>& OutPlacements)
{
	OutPlacements.Reset();
	Rejections.Reset();
	LastError.Reset();
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

bool APPSpawnDirector::PreviewLayout(int32 Seed)
{
	if (GetWorld()->IsGameWorld()) return false;
	ActiveSeed = Seed;
	const bool bSuccess = BuildLayout(Seed, GetPreviewPlayerLocation(GetWorld()), Placements);
	if (bDrawSpawnDebug)
	{
		for (const FPPSpawnPlacement& P : Placements) DrawDebugSphere(GetWorld(), P.Transform.GetLocation(), 100, 12, FColor::Green, false, 30);
	}
	return bSuccess;
}

bool APPSpawnDirector::PreparePatrol(AActor* Player)
{
	if (bPreparationAttempted) return bPrepared;
	const UEnvironmentLevelSubsystem* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>();
	if (!Player || (Rules && Rules->IsTutorialMode())) return false;
	bPreparationAttempted = true;
	ActiveSeed = FixedSeed != 0 ? FixedSeed : static_cast<int32>(FPlatformTime::Cycles());
	if (!BuildLayout(ActiveSeed, Player->GetActorLocation(), Placements))
	{
		UE_LOG(LogTemp, Error, TEXT("Patrol spawn setup failed (seed %d): %s"), ActiveSeed, *LastError);
		return false;
	}
	// Validate every destination before moving anyone. Keep per-instance Blueprint configuration and roster identity.
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
	return true;
}
