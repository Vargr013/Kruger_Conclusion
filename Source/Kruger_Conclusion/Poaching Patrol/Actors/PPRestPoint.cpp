#include "Actors/PPRestPoint.h"

#include "Characters/ARangerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

APPRestPoint::APPRestPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
	VehicleMesh->SetupAttachment(SceneRoot);
	VehicleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	InteractVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractVolume"));
	InteractVolume->SetupAttachment(SceneRoot);
	InteractVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractVolume->SetCollisionObjectType(ECC_WorldDynamic);
	InteractVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractVolume->SetGenerateOverlapEvents(true);
	ApplyInteractRadius();

	InteractionPrompt = NSLOCTEXT("PPRestPoint", "HoldPrompt", "Hold E to restock");
}

void APPRestPoint::BeginPlay()
{
	Super::BeginPlay();
	ApplyInteractRadius();
	InteractVolume->OnComponentBeginOverlap.AddDynamic(this, &APPRestPoint::OnInteractVolumeBeginOverlap);
	InteractVolume->OnComponentEndOverlap.AddDynamic(this, &APPRestPoint::OnInteractVolumeEndOverlap);
}

void APPRestPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyInteractRadius();
}

void APPRestPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ARangerCharacter* Ranger = OverlappingRanger.Get();
	if (!Ranger)
	{
		SetActorTickEnabled(false);
		ClearHold();
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(Ranger->GetController());
	const bool bHoldingInteract = PlayerController && PlayerController->IsInputKeyDown(EKeys::E);
	AdvanceHold(DeltaTime, bHoldingInteract);
}

bool APPRestPoint::IsRangerInRange() const
{
	return OverlappingRanger.IsValid();
}

bool APPRestPoint::IsRangerInRangeFor(const AActor* Ranger) const
{
	return Ranger && OverlappingRanger.Get() == Ranger;
}

float APPRestPoint::GetHoldProgress() const
{
	if (HoldDuration <= KINDA_SMALL_NUMBER)
	{
		return OverlappingRanger.IsValid() ? 1.0f : 0.0f;
	}

	return FMath::Clamp(HoldTime / HoldDuration, 0.0f, 1.0f);
}

void APPRestPoint::SetOverlappingRanger(ARangerCharacter* Ranger)
{
	if (OverlappingRanger.Get() == Ranger)
	{
		SetActorTickEnabled(Ranger != nullptr);
		return;
	}

	ClearHold();
	OverlappingRanger = Ranger;
	SetActorTickEnabled(Ranger != nullptr);
}

void APPRestPoint::AdvanceHold(float DeltaTime, bool bHoldingInteract)
{
	ARangerCharacter* Ranger = OverlappingRanger.Get();
	if (!Ranger)
	{
		ClearHold();
		return;
	}

	if (!bHoldingInteract)
	{
		ClearHold();
		return;
	}

	if (bCompletedThisHold)
	{
		return;
	}

	HoldTime += FMath::Max(0.0f, DeltaTime);
	if (HoldTime >= HoldDuration)
	{
		HoldTime = HoldDuration;
		Ranger->Resupply();
		bCompletedThisHold = true;
	}
}

void APPRestPoint::OnInteractVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (ARangerCharacter* Ranger = Cast<ARangerCharacter>(OtherActor))
	{
		SetOverlappingRanger(Ranger);
	}
}

void APPRestPoint::OnInteractVolumeEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OverlappingRanger.Get() == OtherActor)
	{
		SetOverlappingRanger(nullptr);
	}
}

void APPRestPoint::ApplyInteractRadius()
{
	if (InteractVolume)
	{
		InteractVolume->SetSphereRadius(InteractRadius);
	}
}

void APPRestPoint::ClearHold()
{
	HoldTime = 0.0f;
	bCompletedThisHold = false;
}
