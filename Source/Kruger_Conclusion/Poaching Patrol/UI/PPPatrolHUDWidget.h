#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PPGameTypes.h"
#include "PPPatrolHUDWidget.generated.h"

class AActor;
class ABaseGun;

UCLASS()
class KRUGER_CONCLUSION_API UPPPatrolHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPPPatrolHUDWidget(const FObjectInitializer& ObjectInitializer);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	FVector2D MinimapBottomLeftOffset = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	float MinimapSize = 118.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	float CompassWidth = 820.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	float CompassTopOffset = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	FVector2D ToolCountOffset = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	FVector2D HealthOffset = FVector2D(24.0f, 74.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Minimap")
	float MinimapWorldRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Minimap")
	int32 MaxMinimapActors = 24;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor PanelColor = FLinearColor(0.02f, 0.025f, 0.02f, 0.72f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor LineColor = FLinearColor(0.76f, 0.82f, 0.74f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor PlayerColor = FLinearColor(0.1f, 0.8f, 0.38f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor PoacherColor = FLinearColor(0.9f, 0.16f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor AnimalColor = FLinearColor(0.95f, 0.72f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor ArrestZoneColor = FLinearColor(0.35f, 0.58f, 1.0f, 1.0f);

	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void DrawMinimap(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawCompass(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawPlayerHealth(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawToolCount(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawObjectives(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawEscortStatus(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;

	ABaseGun* FindCurrentTool() const;
	FVector2D WorldToMinimap(const FVector& WorldLocation, const APawn* PlayerPawn, float MapRadius) const;

	TArray<FPPObjectiveState> CachedObjectives;
	FPPEscortStatus CachedEscortStatus;
	float StatusRefreshAccumulator = 0.0f;
	bool bHasEscortStatus = false;
};
