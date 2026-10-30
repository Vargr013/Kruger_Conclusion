#include "Actors/PPSpawnDirector.h"
#include "Actors/PPArrestZone.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "NavigationData.h"

namespace
{
FBoxSphereBounds AnimalBodyBounds(const UMeshComponent* Mesh, const APPCreatureBase* Creature)
{
	FTransform Transform = Mesh->GetComponentTransform();
	const FQuat Inverse = Creature->GetActorQuat().Inverse();
	Transform.SetLocation(Inverse.RotateVector(Mesh->GetComponentLocation() - Creature->GetActorLocation()));
	Transform.SetRotation(Inverse * Transform.GetRotation());
	// Animated physics bounds differ between editor, headless PIE and cooked play.
	// Use the asset's full body bounds so seed validation has the same footprint everywhere.
	if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Mesh))
	{
		FBoxSphereBounds LocalBounds;
		Skinned->GetPreSkinnedLocalBounds(LocalBounds);
		return LocalBounds.TransformBy(Transform);
	}
	return Mesh->CalcBounds(Transform);
}
}

float APPSpawnDirector::GetBodyRadius(const APPCreatureBase* Creature)
{
	float Radius = Creature->GetCapsuleComponent()->GetScaledCapsuleRadius();
	TInlineComponentArray<UMeshComponent*> Meshes(Creature);
	for (const UMeshComponent* Mesh : Meshes)
	{
		if (Mesh->IsVisible())
		{
			const auto Body = AnimalBodyBounds(Mesh, Creature);
			Radius = FMath::Max(Radius, static_cast<float>(FVector2D(Body.BoxExtent.X, Body.BoxExtent.Y).Size()
				+ FVector2D(Body.Origin.X, Body.Origin.Y).Size()));
		}
	}
	return Radius;
}

bool APPSpawnDirector::ProjectCreatureLocation(APPCreatureBase* Creature, const FVector& Raw, FVector& Out, FName& Reason, const FQuat* Facing) const
{
	Reason = TEXT("OutsideArea");
	const float BodyRadius = GetBodyRadius(Creature);
	if (!IsInsidePlayableArea(Raw, FMath::Max(BoundaryInset, BodyRadius))) return false;
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Navigation) { Reason = TEXT("NoNavigation"); return false; }
	FCollisionQueryParams Query(SCENE_QUERY_STAT(PatrolPopulationGround), false, Creature);
	for (TActorIterator<APPPoacherCharacter> It(GetWorld()); It; ++It)
		if (!It->bTutorialEncounter && !It->bStartEncounterInactive) Query.AddIgnoredActor(*It);
	if (bRandomiseAnimals)
	{
		for (TActorIterator<APPAnimalCharacter> It(GetWorld()); It; ++It)
			if (!It->ActorHasTag(TEXT("PPTutorial"))) Query.AddIgnoredActor(*It);
	}
	const FBox Bounds = SpawnBounds->Bounds.GetBox();
	FHitResult Ground;
	Reason = TEXT("UnsafeGround");
	if (!GetWorld()->LineTraceSingleByObjectType(Ground, FVector(Raw.X, Raw.Y, Bounds.Max.Z),
		FVector(Raw.X, Raw.Y, Bounds.Min.Z), FCollisionObjectQueryParams(ECC_WorldStatic), Query)
		|| !Creature->GetCharacterMovement()->IsWalkable(Ground)) return false;
	if (Ground.GetActor() && (Ground.GetActor()->ActorHasTag(TEXT("PPNoSpawn")) || Ground.GetActor()->ActorHasTag(TEXT("Water")))) return false;
	const FNavAgentProperties& Props = Creature->GetNavAgentPropertiesRef();
	FNavLocation Nav;
	Reason = TEXT("OffNavigation");
	if (!Navigation->ProjectPointToNavigation(Ground.ImpactPoint, Nav, FVector(100, 100, 200), &Props)) return false;
	if (!GetWorld()->LineTraceSingleByObjectType(Ground, Nav.Location + FVector(0, 0, 200), Nav.Location - FVector(0, 0, 250),
		FCollisionObjectQueryParams(ECC_WorldStatic), Query) || !Creature->GetCharacterMovement()->IsWalkable(Ground)
		|| FMath::Abs(Ground.ImpactPoint.Z - Nav.Location.Z) > FMath::Max(50.0f, Creature->GetCharacterMovement()->MaxStepHeight)) return false;
	if (Ground.GetActor() && (Ground.GetActor()->ActorHasTag(TEXT("PPNoSpawn")) || Ground.GetActor()->ActorHasTag(TEXT("Water")))) return false;
	const UCapsuleComponent* Capsule = Creature->GetCapsuleComponent();
	Out = Ground.ImpactPoint + FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight() + 3);
	Reason = TEXT("OutsideArea");
	if (!IsInsidePlayableArea(Out, FMath::Max(BoundaryInset, BodyRadius))) return false;
	Reason = TEXT("ExcludedArea");
	for (const AActor* Area : ExclusionAreas)
		if (IsValid(Area) && Area->GetComponentsBoundingBox(true).ExpandBy(FVector(BodyRadius, BodyRadius,
			Capsule->GetScaledCapsuleHalfHeight())).IsInsideOrOn(Out)) return false;
	Reason = TEXT("BlockedCapsule");
	const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
	if (GetWorld()->OverlapBlockingTestByChannel(Out, FQuat::Identity, Capsule->GetCollisionObjectType(),
		FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Query, Responses)) return false;
	// Visible animal meshes can be much wider than their gameplay capsule. Check the body too.
	if (Cast<APPAnimalCharacter>(Creature))
	{
		Reason = TEXT("BlockedBody");
		Query.AddIgnoredActor(Ground.GetActor());
		TInlineComponentArray<UMeshComponent*> Meshes(Creature);
		for (const UMeshComponent* Mesh : Meshes)
		{
			if (!Mesh->IsVisible()) continue;
			const auto Body = AnimalBodyBounds(Mesh, Creature);
			if (Body.BoxExtent.IsNearlyZero()) continue;
			const FQuat Rotation = Facing ? *Facing : Creature->GetActorQuat();
			if (GetWorld()->OverlapBlockingTestByChannel(Out + Rotation.RotateVector(Body.Origin), Rotation,
				Capsule->GetCollisionObjectType(), FCollisionShape::MakeBox(FVector(Body.BoxExtent.X, Body.BoxExtent.Y,
				FMath::Max(5.0, Body.BoxExtent.Z - 5))), Query, Responses)) return false;
		}
	}
	return true;
}

bool APPSpawnDirector::HasCreatureRoute(const APPCreatureBase* Creature, const FVector& Start, const FVector& End, float MaxLength) const
{
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Navigation) return false;
	const FNavAgentProperties& Props = Creature->GetNavAgentPropertiesRef();
	const ANavigationData* Data = Navigation->GetNavDataForProps(Props, Start);
	if (!Data) return false;
	FPathFindingQuery Query(Creature, *Data, Start, End);
	Query.SetAllowPartialPaths(false).SetNavAgentProperties(Props);
	const FPathFindingResult Result = Navigation->FindPathSync(Props, Query);
	if (!Result.IsSuccessful() || !Result.Path.IsValid() || Result.Path->IsPartial() || Result.Path->GetLength() > MaxLength) return false;
	if (Cast<APPAnimalCharacter>(Creature))
	{
		// Existing navigation can be narrower than the rendered animal. Reject routes that clip its body.
		const auto& Points = Result.Path->GetPathPoints();
		for (int32 I = 1; I < Points.Num(); ++I)
		{
			const FVector Direction = Points[I].Location - Points[I - 1].Location;
			const FQuat Facing = FRotator(0, Direction.Rotation().Yaw, 0).Quaternion();
			const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector::Dist2D(Points[I - 1].Location, Points[I].Location) / 200.0f));
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				FVector Projected; FName Reason;
				if (!ProjectCreatureLocation(const_cast<APPCreatureBase*>(Creature), FMath::Lerp(Points[I - 1].Location,
					Points[I].Location, static_cast<float>(Step) / Steps), Projected, Reason, &Facing)) return false;
			}
		}
	}
	return true;
}

bool APPSpawnDirector::PartitionPopulation(int32 Total, int32 Minimum, int32 Maximum, FRandomStream& Random, TArray<int32>& Sizes)
{
	Sizes.Reset();
	if (Total == 0) return true;
	if (Minimum < 1 || Maximum < Minimum || Maximum > 12 || Total < Minimum) return false;
	const int32 MinGroups = FMath::DivideAndRoundUp(Total, Maximum);
	const int32 MaxGroups = Total / Minimum;
	if (MinGroups > MaxGroups) return false;
	const int32 Count = Random.RandRange(MinGroups, MaxGroups);
	Sizes.Init(Minimum, Count);
	int32 Remaining = Total - Count * Minimum;
	while (Remaining > 0)
	{
		const int32 Index = Random.RandRange(0, Count - 1);
		if (Sizes[Index] < Maximum) { ++Sizes[Index]; --Remaining; }
	}
	return true;
}

int32 APPSpawnDirector::FindAnimalProfile(const APPAnimalCharacter* Animal) const
{
	for (int32 I = 0; I < AnimalProfiles.Num(); ++I)
	{
		const FPPAnimalSpawnProfile& Profile = AnimalProfiles[I];
		if (Animal->IsPredator() == Profile.bPredator && (!Animal->GetAnimalSpeciesTag().IsValid()
			|| Profile.Species == Animal->GetAnimalSpeciesTag())) return I;
	}
	return INDEX_NONE;
}

bool APPSpawnDirector::ValidateAnimalPosition(APPAnimalCharacter* Animal, const FVector& Raw, const FVector& PlayerLocation,
	int32 ProfileIndex, int32 GroupId, FVector& OutLocation, FName& Reason) const
{
	if (!ProjectCreatureLocation(Animal, Raw, OutLocation, Reason)) return false;
	const FPPAnimalSpawnProfile& Profile = AnimalProfiles[ProfileIndex];
	Reason = TEXT("AnimalNearPlayer");
	if (FVector::DistSquared2D(OutLocation, PlayerLocation) < FMath::Square(Profile.PlayerSafeDistance)) return false;
	Reason = TEXT("AnimalNearCamp");
	for (TActorIterator<APPArrestZone> It(GetWorld()); It; ++It)
		if (!It->ActorHasTag(TEXT("PPTutorial")) && FVector::DistSquared2D(OutLocation, It->GetActorLocation()) < FMath::Square(Profile.CampSafeDistance)) return false;
	Reason = TEXT("AnimalSpacing");
	for (const FPPAnimalSpawnPlacement& Other : AnimalPlacements)
	{
		const FPPAnimalSpawnProfile& OtherProfile = AnimalProfiles[Other.ProfileIndex];
		float Separation = GetBodyRadius(Animal) + GetBodyRadius(Other.Animal) + 100;
		Separation = FMath::Max(Separation, FMath::Max(Profile.MemberSpacing, OtherProfile.MemberSpacing));
		if (Profile.bPredator != OtherProfile.bPredator) Separation = FMath::Max(Separation, FMath::Max(Profile.ThreatSeparation, OtherProfile.ThreatSeparation));
		if (FVector::DistSquared2D(OutLocation, Other.Transform.GetLocation()) < FMath::Square(Separation)) return false;
	}
	Reason = TEXT("AnimalNoRoamingSpace");
	int32 Probes = 0;
	for (const FVector& Offset : {FVector(400, 0, 0), FVector(-400, 0, 0), FVector(0, 400, 0), FVector(0, -400, 0)})
	{
		FNavLocation Probe;
		UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
		if (Nav->ProjectPointToNavigation(OutLocation + Offset, Probe, FVector(100, 100, 250), &Animal->GetNavAgentPropertiesRef())
			&& IsInsidePlayableArea(Probe.Location, BoundaryInset) && HasCreatureRoute(Animal, OutLocation, Probe.Location, 1500)) ++Probes;
	}
	return Probes >= 2;
}

bool APPSpawnDirector::BuildAnimalLayout(int32 Seed, const FVector& PlayerLocation, bool bAuthoredFallback)
{
	AnimalPlacements.Reset();
	AnimalGroups.Reset();
	if (!bRandomiseAnimals) return true;
	TArray<APPAnimalCharacter*> Roster;
	for (TActorIterator<APPAnimalCharacter> It(GetWorld()); It; ++It)
		if (!It->ActorHasTag(TEXT("PPTutorial"))) Roster.Add(*It);
	Roster.Sort([](const APPAnimalCharacter& A, const APPAnimalCharacter& B) { return A.GetName() < B.GetName(); });
	for (APPAnimalCharacter* Animal : Roster)
		if (FindAnimalProfile(Animal) == INDEX_NONE) { LastError = TEXT("An animal has no matching species spawn profile."); return false; }
	FRandomStream Random(Seed);
	const FVector Extent = SpawnBounds->GetUnscaledBoxExtent();
	TArray<int32> ProfileOrder;
	for (int32 Index = 0; Index < AnimalProfiles.Num(); ++Index) ProfileOrder.Add(Index);
	// Reserve space for herds, then solitary herbivores, before placing predators.
	ProfileOrder.StableSort([&](int32 A, int32 B)
	{
		const auto Rank = [](const FPPAnimalSpawnProfile& P) { return P.bPredator ? 2 : P.bHerding ? 0 : 1; };
		return Rank(AnimalProfiles[A]) < Rank(AnimalProfiles[B]);
	});
	for (int32 ProfileIndex : ProfileOrder)
	{
		const FPPAnimalSpawnProfile& Profile = AnimalProfiles[ProfileIndex];
		TArray<APPAnimalCharacter*> Species;
		for (APPAnimalCharacter* Animal : Roster) if (FindAnimalProfile(Animal) == ProfileIndex) Species.Add(Animal);
		TArray<int32> Sizes;
		if (!PartitionPopulation(Species.Num(), Profile.bHerding ? Profile.MinGroupSize : 1,
			Profile.bHerding ? Profile.MaxGroupSize : 1, Random, Sizes))
		{ LastError = TEXT("Animal group sizes cannot divide the species population."); return false; }
		if ((Profile.bPredator && Profile.bHerding) || Profile.SpreadRadius <= 0 || Profile.MemberSpacing <= 0
			|| Profile.RoamRadius <= 0 || Profile.RegroupDistance < Profile.SpreadRadius || Profile.AlertDistance <= 0
			|| Profile.CalmSeconds < 1 || Profile.UpdateSeconds < 0.5f)
		{ LastError = TEXT("Animal spawn profile contains invalid distances or timing."); return false; }
		// Fit the largest groups before smaller groups consume the open clearings.
		Sizes.Sort([](int32 A, int32 B) { return A > B; });
		int32 MemberIndex = 0;
		for (int32 Size : Sizes)
		{
			float FormationSpacing = Profile.MemberSpacing;
			for (int32 Member = 0; Member < Size; ++Member)
				FormationSpacing = FMath::Max(FormationSpacing, GetBodyRadius(Species[MemberIndex + Member]) * 2 + 100);
			const float FormationRadius = Size > 1 ? FMath::Max(Profile.SpreadRadius * 0.55f,
				FormationSpacing / (2 * FMath::Sin(PI / Size)) * 1.15f) : 0;
			const int32 GroupId = AnimalGroups.Num();
			const int32 FirstPlacement = AnimalPlacements.Num();
			bool bGroupPlaced = false;
			for (int32 Attempt = 0; Attempt < (bAuthoredFallback ? 1 : FMath::Clamp(AttemptsPerRegion, 1, 500) * 2); ++Attempt)
			{
				AnimalPlacements.SetNum(FirstPlacement);
				FVector RawCenter = SpawnBounds->GetComponentTransform().TransformPosition(FVector(
					Random.FRandRange(-Extent.X, Extent.X), Random.FRandRange(-Extent.Y, Extent.Y), 0));
				if (bAuthoredFallback)
				{
					RawCenter = FVector::ZeroVector;
					for (int32 M = 0; M < Size; ++M) RawCenter += Species[MemberIndex + M]->GetActorLocation();
					RawCenter /= Size;
				}
				FVector Center; FName Reason;
				if (!ValidateAnimalPosition(Species[MemberIndex], RawCenter, PlayerLocation, ProfileIndex, GroupId, Center, Reason))
				{ RecordRejection(RawCenter, Reason); continue; }
				bool bTooClose = false;
				for (const FPPAnimalGroupPlan& Group : AnimalGroups)
				{
					const FPPAnimalSpawnProfile& Other = AnimalProfiles[Group.ProfileIndex];
					if (Profile.bPredator == Other.bPredator && FVector::DistSquared2D(Center, Group.Center)
						< FMath::Square(FMath::Max(Profile.GroupSpacing, Other.GroupSpacing))) bTooClose = true;
				}
				if (bTooClose) { RecordRejection(Center, TEXT("GroupSpacing")); continue; }
				const float FormationAngle = Random.FRandRange(0, 2 * PI);
				for (int32 Member = 0; Member < Size; ++Member)
				{
					APPAnimalCharacter* Animal = Species[MemberIndex + Member];
					bool bMemberPlaced = false;
					for (int32 Slot = 0; Slot < (bAuthoredFallback ? 1 : 40); ++Slot)
					{
						// Give each member a separate sector so early random slots cannot block the rest.
						// The centre, facing and small slot variations still change with the seed.
						const float Angle = FormationAngle + Member * 2 * PI / Size
							+ (Slot == 0 ? 0 : Random.FRandRange(-PI / Size * 0.35f, PI / Size * 0.35f));
						const float Radius = Size == 1 ? 0 : FMath::Min(Profile.SpreadRadius,
							FormationRadius * (Slot == 0 ? 1 : Random.FRandRange(0.95f, 1.2f)));
						const FVector Raw = bAuthoredFallback ? Animal->GetActorLocation() : Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0);
						FVector Position;
						if (!ValidateAnimalPosition(Animal, Raw, PlayerLocation, ProfileIndex, GroupId, Position, Reason)
							|| FVector::DistSquared2D(Position, Center) > FMath::Square(Profile.SpreadRadius + 100)
							|| !HasCreatureRoute(Animal, Position, Center, Profile.SpreadRadius * 4)
							|| !HasCreatureRoute(Animal, Center, Position, Profile.SpreadRadius * 4))
						{ RecordRejection(Raw, Reason); continue; }
						FPPAnimalSpawnPlacement Placement;
						Placement.Animal = Animal; Placement.GroupId = GroupId; Placement.ProfileIndex = ProfileIndex;
						Placement.bUsedFallback = bAuthoredFallback;
						Placement.Transform = FTransform(Animal->GetActorRotation(), Position, Animal->GetActorScale3D());
						AnimalPlacements.Add(Placement);
						bMemberPlaced = true;
						break;
					}
					if (!bMemberPlaced) break;
				}
				if (AnimalPlacements.Num() == FirstPlacement + Size)
				{
					FPPAnimalGroupPlan Group; Group.Center = Center; Group.ProfileIndex = ProfileIndex;
					AnimalGroups.Add(Group); bGroupPlaced = true; break;
				}
			}
			if (!bGroupPlaced) { LastError = FString::Printf(TEXT("No safe space for animal group %d."), GroupId); return false; }
			MemberIndex += Size;
		}
	}
	return true;
}
