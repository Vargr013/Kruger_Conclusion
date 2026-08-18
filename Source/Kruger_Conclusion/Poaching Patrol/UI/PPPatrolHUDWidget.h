#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PPGameTypes.h"
#include "PPPatrolHUDWidget.generated.h"

class AActor;
class ABaseGun;
class APPAnimalCharacter;
class APPArrestZone;
class APPPoacherCharacter;
class UPPMinimapDefinition;

UCLASS()
class KRUGER_CONCLUSION_API UPPPatrolHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPPPatrolHUDWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol HUD|Minimap")
	void CycleMinimapZoom();

	UFUNCTION(BlueprintPure, Category="Poaching Patrol HUD|Minimap")
	float GetCurrentMinimapWorldRadius() const;

	UFUNCTION(BlueprintPure, Category="Poaching Patrol HUD|Minimap")
	int32 GetCurrentMinimapZoomIndex() const { return CurrentMinimapZoomIndex; }

	static FVector2D ProjectWorldToMinimap(
		const FVector& WorldLocation,
		const FVector& PlayerLocation,
		float ViewYawDegrees,
		float MapRadius,
		float WorldRadius);

	static FVector2D ClampMinimapPointToSquare(const FVector2D& Point, float HalfExtent);

	static bool ProjectWorldToCompass(
		const FVector& WorldLocation,
		const FVector& PlayerLocation,
		float ViewYawDegrees,
		float VisibleDegrees,
		float MaxWorldDistance,
		float& OutNormalizedOffset);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	FVector2D MinimapBottomLeftOffset = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	float MinimapSize = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	float CompassWidth = 820.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	float CompassTopOffset = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Compass", meta=(ClampMin="0.0"))
	float CompassPoacherRange = 14000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Compass", meta=(ClampMin="2.0", ClampMax="24.0"))
	float CompassPoacherMarkerSize = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	FVector2D ToolCountOffset = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Layout")
	FVector2D HealthOffset = FVector2D(24.0f, 74.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Minimap")
	TArray<float> MinimapZoomRadii = {4000.0f, 8000.0f, 14000.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Minimap")
	TObjectPtr<UPPMinimapDefinition> MinimapDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor PanelColor = FLinearColor(0.02f, 0.025f, 0.02f, 0.72f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor LineColor = FLinearColor(0.76f, 0.82f, 0.74f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor PlayerColor = FLinearColor(0.1f, 0.8f, 0.38f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor PoacherColor = FLinearColor(0.9f, 0.16f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Poaching Patrol HUD|Style")
	FLinearColor EscortedPoacherColor = FLinearColor(0.96f, 0.62f, 0.22f, 1.0f);

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
	void DrawControlsHint(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawCompass(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawPlayerHealth(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawToolCount(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawObjectives(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawEscortStatus(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;
	void DrawCombatStatus(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const;

	ABaseGun* FindCurrentTool() const;
	float GetResponsiveMinimapSize(const FVector2D& ViewSize) const;
	FVector2D WorldToMinimap(const FVector& WorldLocation, const APawn* PlayerPawn, float MapRadius) const;
	void RefreshMinimapActors();

	TArray<FPPObjectiveState> CachedObjectives;
	FPPEscortStatus CachedEscortStatus;
	float StatusRefreshAccumulator = 0.0f;
	bool bHasEscortStatus = false;
	int32 CurrentMinimapZoomIndex = 1;
	TArray<TWeakObjectPtr<APPPoacherCharacter>> CachedMinimapPoachers;
	TArray<TWeakObjectPtr<APPAnimalCharacter>> CachedMinimapAnimals;
	TArray<TWeakObjectPtr<APPArrestZone>> CachedArrestZones;
	TWeakObjectPtr<APPPoacherCharacter> CachedUrgentAttacker;
	int32 CachedHostilePoacherCount = 0;
	float DamageFlashRemaining = 0.0f;
	float LastObservedPlayerHealth = -1.0f;
};
