#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PPGameTypes.h"
#include "PPRestraintMinigameWidget.generated.h"

class APPPoacherCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPPRestraintFinished, EPPRestraintResult, Result);

UCLASS()
class KRUGER_CONCLUSION_API UPPRestraintMinigameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPPRestraintMinigameWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Restraint")
	void StartSession(APPPoacherCharacter* InPoacher, bool bInPepperAssisted);

	UPROPERTY(BlueprintAssignable, Category="Poaching Patrol|Restraint")
	FOnPPRestraintFinished OnRestraintFinished;

	static bool IsHitTimingValid(double ElapsedSeconds, double ApproachDuration, double WindowDuration);
	static bool CanStillSucceed(int32 Hits, int32 TargetsProcessed, int32 TotalTargets, int32 RequiredHits);
	static float GetApproachDurationForAssist(bool bPepperAssisted, float NormalDuration, float AssistedDuration);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Poaching Patrol|Restraint", meta=(ClampMin="1"))
	int32 TargetCount = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Poaching Patrol|Restraint", meta=(ClampMin="1"))
	int32 RequiredHits = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Poaching Patrol|Restraint", meta=(ClampMin="16.0"))
	float TargetRadiusAt1080p = 72.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Poaching Patrol|Restraint", meta=(ClampMin="0.1"))
	float NormalApproachDuration = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Poaching Patrol|Restraint", meta=(ClampMin="0.1"))
	float PepperAssistedApproachDuration = 1.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Poaching Patrol|Restraint", meta=(ClampMin="0.01"))
	float HitWindowDuration = 0.50f;

	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildPattern();
	void RecordHit(double CurrentRealTime);
	void RecordMiss(double CurrentRealTime);
	void AdvanceOrFinish(double CurrentRealTime);
	void QueueCompletion(EPPRestraintResult Result, double CurrentRealTime);
	FVector2D GetActiveTargetCenter(const FVector2D& LocalSize) const;
	float GetScaledTargetRadius(const FVector2D& LocalSize) const;
	void DrawRing(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Radius,
		const FLinearColor& Color,
		float Thickness) const;

	TWeakObjectPtr<APPPoacherCharacter> TargetPoacher;
	TArray<FVector2D> ActivePattern;
	bool bPepperAssisted = false;
	bool bSessionActive = false;
	bool bPendingCompletion = false;
	int32 ActiveTargetIndex = 0;
	int32 Hits = 0;
	int32 Misses = 0;
	double CurrentTargetStartTime = 0.0;
	double FeedbackExpiresAt = 0.0;
	double CompletionBroadcastTime = 0.0;
	EPPRestraintResult PendingResult = EPPRestraintResult::Failed;
	FString FeedbackText;
	mutable FVector2D CachedLocalSize = FVector2D(1920.0f, 1080.0f);
};
