#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPTutorialTarget.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API APPTutorialTarget : public AActor
{
	GENERATED_BODY()
public:
	APPTutorialTarget();
	virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* EventInstigator, AActor* Causer) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UStaticMeshComponent> TargetMesh;
};
