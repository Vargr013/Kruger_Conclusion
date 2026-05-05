#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPArrestZone.generated.h"

class UBoxComponent;

UCLASS()
class KRUGER_CONCLUSION_API APPArrestZone : public AActor
{
	GENERATED_BODY()

public:
	APPArrestZone();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Arrest Zone")
	UBoxComponent* ArrestBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone")
	FVector ZoneExtent = FVector(300.0f, 300.0f, 150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Debug")
	bool bDrawDebug = true;

	UFUNCTION()
	void OnArrestBoundsBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

public:
	UFUNCTION(BlueprintCallable, Category="Arrest Zone")
	UBoxComponent* GetArrestBounds() const { return ArrestBounds; }
};
