#include "Actors/PPVegetationPatch.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

APPVegetationPatch::APPVegetationPatch()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	Bushes = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Bushes"));
	GrassOne = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassOne"));
	GrassTwo = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassTwo"));
	GrassThree = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassThree"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BushMesh(TEXT("/Game/Assets/Bush"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Grass1(TEXT("/Game/Foliage/Grass/Grass1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Grass2(TEXT("/Game/Foliage/Grass/Grass2"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Grass3(TEXT("/Game/Foliage/Grass/Grass3"));
	Bushes->SetStaticMesh(BushMesh.Object);
	GrassOne->SetStaticMesh(Grass1.Object);
	GrassTwo->SetStaticMesh(Grass2.Object);
	GrassThree->SetStaticMesh(Grass3.Object);
	for (UHierarchicalInstancedStaticMeshComponent* Mesh : {Bushes.Get(), GrassOne.Get(), GrassTwo.Get(), GrassThree.Get()})
	{
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetMobility(EComponentMobility::Static);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->bEnableDensityScaling = false;
		Mesh->SetCullDistances(0, 0);
	}
}

void APPVegetationPatch::AddBush(const FTransform& WorldTransform)
{
	Bushes->AddInstance(WorldTransform, true);
}

void APPVegetationPatch::AddGrass(int32 Variant, const FTransform& WorldTransform)
{
	UHierarchicalInstancedStaticMeshComponent* Mesh = Variant % 3 == 0 ? GrassOne.Get() : Variant % 3 == 1 ? GrassTwo.Get() : GrassThree.Get();
	Mesh->AddInstance(WorldTransform, true);
}

void APPVegetationPatch::AddSightCore(FVector WorldCentre, FVector Extent, float Yaw)
{
	UBoxComponent* Core = NewObject<UBoxComponent>(this, NAME_None, RF_Transactional);
	AddInstanceComponent(Core);
	Core->SetupAttachment(RootComponent);
	Core->SetMobility(EComponentMobility::Static);
	Core->SetBoxExtent(Extent);
	Core->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Core->SetCollisionResponseToAllChannels(ECR_Ignore);
	Core->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	Core->SetGenerateOverlapEvents(false);
	Core->SetCanEverAffectNavigation(false);
	Core->ShapeColor = FColor::Green;
	Core->bDrawOnlyIfSelected = true;
	Core->SetRelativeTransform(FTransform(FRotator(0, Yaw, 0), WorldCentre).GetRelativeTransform(GetActorTransform()));
	Core->RegisterComponent();
	SightCores.Add(Core);
}

void APPVegetationPatch::ClearPatch()
{
	for (UBoxComponent* Core : SightCores)
		if (IsValid(Core)) { RemoveInstanceComponent(Core); Core->DestroyComponent(); }
	SightCores.Reset();
	for (UHierarchicalInstancedStaticMeshComponent* Mesh : {Bushes.Get(), GrassOne.Get(), GrassTwo.Get(), GrassThree.Get()}) Mesh->ClearInstances();
}

int32 APPVegetationPatch::GetInstanceCount() const
{
	return Bushes->GetInstanceCount() + GrassOne->GetInstanceCount() + GrassTwo->GetInstanceCount() + GrassThree->GetInstanceCount();
}

bool APPVegetationPatch::HasProtectedCoverSettings() const
{
	for (const UHierarchicalInstancedStaticMeshComponent* Mesh : {Bushes.Get(), GrassOne.Get(), GrassTwo.Get(), GrassThree.Get()})
	{
		if (Mesh->bEnableDensityScaling || Mesh->InstanceEndCullDistance != 0
			|| Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision || Mesh->CanEverAffectNavigation()) return false;
	}
	return true;
}
