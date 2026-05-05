#include "Actors/PPArrestZone.h"

#include "Characters/PPPoacherCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

APPArrestZone::APPArrestZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ArrestBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ArrestBounds"));
	SetRootComponent(ArrestBounds);

	ArrestBounds->SetBoxExtent(ZoneExtent);
	ArrestBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ArrestBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ArrestBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ArrestBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ArrestBounds->SetGenerateOverlapEvents(true);
}

void APPArrestZone::BeginPlay()
{
	Super::BeginPlay();
	ArrestBounds->OnComponentBeginOverlap.AddDynamic(this, &APPArrestZone::OnArrestBoundsBeginOverlap);
}

void APPArrestZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (ArrestBounds)
	{
		ArrestBounds->SetBoxExtent(ZoneExtent);
	}
}

void APPArrestZone::OnArrestBoundsBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APPPoacherCharacter* Poacher = Cast<APPPoacherCharacter>(OtherActor);
	if (!Poacher)
	{
		return;
	}

	const EPPPoacherState PoacherState = Poacher->GetPoacherState();
	const bool bCanArrest = Poacher->IsCaptured()
		|| PoacherState == EPPPoacherState::Captured
		|| PoacherState == EPPPoacherState::FollowingPlayer;

	if (!bCanArrest)
	{
		return;
	}

	Poacher->MarkArrested();

	if (bDrawDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s arrested in %s"), *Poacher->GetName(), *GetName());
		DrawDebugBox(GetWorld(), GetActorLocation(), ZoneExtent, GetActorQuat(), FColor::Green, false, 3.0f, 0, 5.0f);
	}
}
