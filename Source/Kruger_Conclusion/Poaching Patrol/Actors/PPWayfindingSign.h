#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PPWayfindingSign.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/** Reusable stone sign with instance-editable, runtime-changeable panel text. */
UCLASS()
class KRUGER_CONCLUSION_API APPWayfindingSign : public AActor
{
	GENERATED_BODY()

public:
	APPWayfindingSign();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="Wayfinding")
	void SetSignText(const FText& NewText);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wayfinding", meta=(MultiLine=true))
	FText SignText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wayfinding")
	TObjectPtr<UStaticMeshComponent> SignMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wayfinding")
	TObjectPtr<UTextRenderComponent> PanelText;

private:
	void UpdatePanelText();
};
