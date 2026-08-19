#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPRestPoint.generated.h"

class ARangerCharacter;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class KRUGER_CONCLUSION_API APPRestPoint : public AActor
{
	GENERATED_BODY()

public:
	APPRestPoint();

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	bool IsRangerInRange() const;

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	bool IsRangerInRangeFor(const AActor* Ranger) const;

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	float GetHoldProgress() const;

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	FText GetInteractionPrompt() const { return InteractionPrompt; }

	UFUNCTION(BlueprintCallable, Category = "Rest Point")
	void SetOverlappingRanger(ARangerCharacter* Ranger);

	UFUNCTION(BlueprintCallable, Category = "Rest Point")
	void AdvanceHold(float DeltaTime, bool bHoldingInteract);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rest Point")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rest Point")
	UStaticMeshComponent* VehicleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rest Point")
	USphereComponent* InteractVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest Point", meta = (ClampMin = "0.1"))
	float HoldDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest Point", meta = (ClampMin = "1.0"))
	float InteractRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest Point")
	FText InteractionPrompt;

	UFUNCTION()
	void OnInteractVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractVolumeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	void ApplyInteractRadius();
	void ClearHold();

	UPROPERTY()
	TWeakObjectPtr<ARangerCharacter> OverlappingRanger;

	float HoldTime = 0.0f;
	bool bCompletedThisHold = false;
};
