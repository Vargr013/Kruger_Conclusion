#include "Actors/PPArrestZone.h"

#include "EnvironmentLevelSubsystem.h"
#include "Characters/PPPoacherCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "NiagaraComponent.h"

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

	FlareLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlareLight"));
	FlareLight->SetupAttachment(ArrestBounds);
	FlareLight->SetLightColor(FlareColor);
	FlareLight->SetIntensity(FlareLightIntensity);
	FlareLight->SetAttenuationRadius(FlareLightRadius);
	FlareLight->SetRelativeLocation(FVector(0.0f, 0.0f, FlareHeight));

	FlareEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlareEffect"));
	FlareEffectComponent->SetupAttachment(ArrestBounds);
	FlareEffectComponent->SetRelativeLocation(FVector(0.0f, 0.0f, FlareHeight));
	FlareEffectComponent->SetAutoActivate(true);
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

	if (FlareLight)
	{
		FlareLight->SetVisibility(bShowZoneMarker);
		FlareLight->SetHiddenInGame(!bShowZoneMarker);
		FlareLight->SetRelativeLocation(FVector(0.0f, 0.0f, FlareHeight));
		FlareLight->SetLightColor(FlareColor);
		FlareLight->SetIntensity(FlareLightIntensity);
		FlareLight->SetAttenuationRadius(FlareLightRadius);
	}

	if (FlareEffectComponent)
	{
		FlareEffectComponent->SetAsset(FlareEffect);
		FlareEffectComponent->SetVisibility(bShowZoneMarker && FlareEffect != nullptr);
		FlareEffectComponent->SetHiddenInGame(!bShowZoneMarker || FlareEffect == nullptr);
		FlareEffectComponent->SetRelativeLocation(FVector(0.0f, 0.0f, FlareHeight));
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
	const bool bCanArrest = PoacherState == EPPPoacherState::Captured
		|| PoacherState == EPPPoacherState::FollowingPlayer;

	if (!bCanArrest)
	{
		if (bPlayZoneDebugMessages && bDrawDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s entered %s but is not capturable/following"), *Poacher->GetName(), *GetName());
		}
		return;
	}

	// Attempt to mark the poacher as arrested. If this returns false, it means the poacher was already marked as arrested, so we can skip the rest of the logic to avoid double counting captures or triggering multiple capture events.
	const bool bWasArrested = Poacher->MarkArrested();
	if (!bWasArrested)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UEnvironmentLevelSubsystem* LevelSubsystem = World->GetSubsystem<UEnvironmentLevelSubsystem>())
		{
			LevelSubsystem->OnPoacherCaptured();
		}
	}

	if (bRemovePoacherAfterArrest)
	{
		Poacher->RemoveFromLevelAfterArrest(PoacherRemovalDelay);
	}

	if (bPlayZoneDebugMessages && bDrawDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s arrested in %s"), *Poacher->GetName(), *GetName());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("%s arrested"), *Poacher->GetName()));
		}
		DrawDebugBox(GetWorld(), GetActorLocation(), ZoneExtent, GetActorQuat(), FColor::Green, false, 3.0f, 0, 5.0f);
	}
}
