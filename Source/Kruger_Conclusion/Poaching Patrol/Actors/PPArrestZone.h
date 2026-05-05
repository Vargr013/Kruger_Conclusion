#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPArrestZone.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Arrest Zone|Marker")
	UPointLightComponent* FlareLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Arrest Zone|Marker")
	UNiagaraComponent* FlareEffectComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone")
	FVector ZoneExtent = FVector(300.0f, 300.0f, 150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone")
	bool bRemovePoacherAfterArrest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone")
	float PoacherRemovalDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Marker")
	bool bShowZoneMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Marker")
	UNiagaraSystem* FlareEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Marker")
	float FlareHeight = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Marker")
	float FlareLightIntensity = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Marker")
	float FlareLightRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Marker")
	FLinearColor FlareColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Arrest Zone|Debug")
	bool bPlayZoneDebugMessages = true;

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
