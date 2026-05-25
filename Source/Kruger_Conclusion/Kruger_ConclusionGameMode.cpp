// Copyright Epic Games, Inc. All Rights Reserved.

#include "Kruger_ConclusionGameMode.h"

#include "Actors/PPArrestZone.h"
#include "Characters/PPCreatureBase.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"

AKruger_ConclusionGameMode::AKruger_ConclusionGameMode()
{
	// stub
}

void AKruger_ConclusionGameMode::StartPlay()
{
	Super::StartPlay();
	EnsureBuildReadyNavigation();
}

void AKruger_ConclusionGameMode::EnsureBuildReadyNavigation()
{
	UWorld* World = GetWorld();
	if (!World || World->IsNetMode(NM_Client))
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return;
	}

	for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It)
	{
		NavSys->GetDefaultNavDataInstance(FNavigationSystem::Create);
		NavSys->Build();
		return;
	}

	FBox RuntimeNavBounds(ForceInit);
	auto AddActorBounds = [&RuntimeNavBounds](const AActor* Actor)
	{
		if (!Actor)
		{
			return;
		}

		FVector Origin = FVector::ZeroVector;
		FVector Extent = FVector::ZeroVector;
		Actor->GetActorBounds(false, Origin, Extent);
		Extent = Extent.ComponentMax(FVector(250.0f));
		RuntimeNavBounds += Origin - Extent;
		RuntimeNavBounds += Origin + Extent;
	};

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		AddActorBounds(*It);
	}

	for (TActorIterator<APPCreatureBase> It(World); It; ++It)
	{
		AddActorBounds(*It);
	}

	for (TActorIterator<APPArrestZone> It(World); It; ++It)
	{
		AddActorBounds(*It);
	}

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		AddActorBounds(*It);
	}

	if (!RuntimeNavBounds.IsValid)
	{
		RuntimeNavBounds = FBox(FVector(-5000.0f, -5000.0f, -500.0f), FVector(5000.0f, 5000.0f, 1500.0f));
	}

	RuntimeNavBounds = RuntimeNavBounds.ExpandBy(FVector(3000.0f, 3000.0f, 1200.0f));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = TEXT("RuntimeBuild_NavMeshBounds");
	SpawnParams.ObjectFlags |= RF_Transient;

	const FVector Center = RuntimeNavBounds.GetCenter();
	const FVector Extent = RuntimeNavBounds.GetExtent();
	ANavMeshBoundsVolume* NavBounds = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), Center, FRotator::ZeroRotator, SpawnParams);
	if (!NavBounds)
	{
		return;
	}

	NavBounds->SetActorScale3D(FVector(
		FMath::Max(1.0f, Extent.X / 100.0f),
		FMath::Max(1.0f, Extent.Y / 100.0f),
		FMath::Max(1.0f, Extent.Z / 100.0f)));

	NavSys->OnNavigationBoundsUpdated(NavBounds);
	NavSys->GetDefaultNavDataInstance(FNavigationSystem::Create);
	NavSys->Build();
}
