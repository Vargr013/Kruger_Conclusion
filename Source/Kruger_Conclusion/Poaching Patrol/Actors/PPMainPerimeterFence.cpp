#include "Actors/PPMainPerimeterFence.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

APPMainPerimeterFence::APPMainPerimeterFence()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Bars = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FenceBars"));
	Bars->SetupAttachment(SceneRoot);
	Bars->SetCollisionProfileName(TEXT("NoCollision"));
	Bars->SetGenerateOverlapEvents(false);
	Bars->SetCanEverAffectNavigation(false);

	Mesh = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FenceMesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BarsAsset(TEXT("/Game/Assets/Human_Stuff/Fence/FenceBars.FenceBars"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Assets/Human_Stuff/Fence/FenceMesh1.FenceMesh1"));
	if (BarsAsset.Succeeded()) Bars->SetStaticMesh(BarsAsset.Object);
	if (MeshAsset.Succeeded()) Mesh->SetStaticMesh(MeshAsset.Object);

	Tags.Add(TEXT("PPMainPerimeter"));
}

void APPMainPerimeterFence::AddSection(const FTransform& WorldTransform)
{
	Bars->AddInstance(WorldTransform, true);
	Mesh->AddInstance(WorldTransform, true);
}

void APPMainPerimeterFence::ClearSections()
{
	Bars->ClearInstances();
	Mesh->ClearInstances();
}

int32 APPMainPerimeterFence::GetSectionCount() const
{
	return Bars->GetInstanceCount();
}
