#include "Actors/PPTutorialTarget.h"
#include "Actors/PPTutorialDirector.h"
#include "EnvironmentLevelSubsystem.h"
#include "BaseGun.h"
#include "BaseProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "UObject/ConstructorHelpers.h"

APPTutorialTarget::APPTutorialTarget()
{
	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PracticeTarget"));
	SetRootComponent(TargetMesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	TargetMesh->SetStaticMesh(Mesh.Object);
	TargetMesh->SetCollisionProfileName(TEXT("BlockAll"));
	TargetMesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.2f));
	TargetMesh->SetRelativeRotation(FRotator(90, 0, 0));
}

float APPTutorialTarget::TakeDamage(float Amount, const FDamageEvent& Event, AController* EventInstigator, AActor* Causer)
{
	if (Amount <= 0 || !EventInstigator || !EventInstigator->IsPlayerController() || (!Cast<ABaseGun>(Causer) && !Cast<ABaseProjectile>(Causer))) return 0;
	if (auto* Rules = GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>())
		if (auto* Director = Rules->GetTutorialDirector()) Director->NotifyPracticeHit(this, EventInstigator);
	return Amount;
}
