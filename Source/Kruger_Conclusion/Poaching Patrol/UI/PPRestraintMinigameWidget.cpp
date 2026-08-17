#include "UI/PPRestraintMinigameWidget.h"

#include "Characters/PPPoacherCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Engine/Engine.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Misc/DefaultValueHelper.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SWidget.h"

namespace
{
	constexpr double ResultDisplayDuration = 0.55;
	constexpr double HitFeedbackDuration = 0.28;

	const TArray<TArray<FVector2D>>& GetRestraintPatterns()
	{
		static const TArray<TArray<FVector2D>> Patterns = {
			{{0.30f, 0.40f}, {0.50f, 0.30f}, {0.70f, 0.42f}, {0.62f, 0.66f}, {0.38f, 0.68f}, {0.50f, 0.50f}},
			{{0.25f, 0.52f}, {0.42f, 0.34f}, {0.64f, 0.32f}, {0.75f, 0.55f}, {0.56f, 0.70f}, {0.36f, 0.66f}},
			{{0.34f, 0.30f}, {0.66f, 0.30f}, {0.72f, 0.56f}, {0.54f, 0.70f}, {0.28f, 0.58f}, {0.48f, 0.48f}}
		};
		return Patterns;
	}
}

UPPRestraintMinigameWidget::UPPRestraintMinigameWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UPPRestraintMinigameWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// The Blueprint intentionally has no authored controls. Give it a concrete,
	// full-screen hit-test surface so native-painted targets receive mouse input.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RestraintInputSurface"));
	}
	SetVisibility(ESlateVisibility::Visible);
	if (WidgetTree && WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPPRestraintMinigameWidget::StartSession(APPPoacherCharacter* InPoacher, bool bInPepperAssisted)
{
	TargetPoacher = InPoacher;
	bPepperAssisted = bInPepperAssisted;
	bSessionActive = true;
	bPendingCompletion = false;
	ActiveTargetIndex = 0;
	Hits = 0;
	Misses = 0;
	FeedbackText.Reset();
	FeedbackExpiresAt = 0.0;
	BuildPattern();
	CurrentTargetStartTime = FPlatformTime::Seconds();
	Invalidate(EInvalidateWidgetReason::Paint);
}

bool UPPRestraintMinigameWidget::IsHitTimingValid(double ElapsedSeconds, double ApproachDuration, double WindowDuration)
{
	return ElapsedSeconds >= ApproachDuration - WindowDuration
		&& ElapsedSeconds <= ApproachDuration + WindowDuration;
}

bool UPPRestraintMinigameWidget::CanStillSucceed(int32 CurrentHits, int32 TargetsProcessed, int32 TotalTargets, int32 HitsRequired)
{
	const int32 RemainingTargets = FMath::Max(0, TotalTargets - TargetsProcessed);
	return CurrentHits + RemainingTargets >= HitsRequired;
}

float UPPRestraintMinigameWidget::GetApproachDurationForAssist(bool bIsPepperAssisted, float NormalDuration, float AssistedDuration)
{
	return bIsPepperAssisted ? AssistedDuration : NormalDuration;
}

void UPPRestraintMinigameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	CachedLocalSize = MyGeometry.GetLocalSize();

	const double CurrentRealTime = FPlatformTime::Seconds();
	if (bPendingCompletion)
	{
		if (CurrentRealTime >= CompletionBroadcastTime)
		{
			bPendingCompletion = false;
			bSessionActive = false;
			OnRestraintFinished.Broadcast(PendingResult);
		}
		Invalidate(EInvalidateWidgetReason::Paint);
		return;
	}

	if (!bSessionActive || ActiveTargetIndex >= TargetCount)
	{
		return;
	}

	const float ApproachDuration = GetApproachDurationForAssist(
		bPepperAssisted,
		NormalApproachDuration,
		PepperAssistedApproachDuration);
	if (CurrentRealTime - CurrentTargetStartTime > ApproachDuration + HitWindowDuration)
	{
		RecordMiss(CurrentRealTime);
	}

	if (!FeedbackText.IsEmpty() && CurrentRealTime >= FeedbackExpiresAt)
	{
		FeedbackText.Reset();
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UPPRestraintMinigameWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	CachedLocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D ViewSize = CachedLocalSize;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		BaseLayer + 1,
		AllottedGeometry.ToPaintGeometry(ViewSize, FSlateLayoutTransform()),
		WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(0.005f, 0.008f, 0.006f, 0.68f));

	const FSlateFontInfo HeadingFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 28);
	const FSlateFontInfo DetailFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 17);
	const FSlateFontInfo NumberFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 24);
	const FLinearColor PaperColor(0.90f, 0.84f, 0.65f, 1.0f);
	const FLinearColor HitColor(0.20f, 0.92f, 0.42f, 1.0f);
	const FLinearColor MissColor(0.96f, 0.22f, 0.12f, 1.0f);
	const FLinearColor TargetColor(0.96f, 0.62f, 0.16f, 1.0f);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		BaseLayer + 2,
		AllottedGeometry.ToPaintGeometry(FVector2D(600.0f, 42.0f), FSlateLayoutTransform(FVector2D((ViewSize.X - 600.0f) * 0.5f, 36.0f))),
		TEXT("RESTRAIN THE POACHER"),
		HeadingFont,
		ESlateDrawEffect::None,
		PaperColor);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		BaseLayer + 2,
		AllottedGeometry.ToPaintGeometry(FVector2D(760.0f, 28.0f), FSlateLayoutTransform(FVector2D((ViewSize.X - 760.0f) * 0.5f, 80.0f))),
		TEXT("CLICK WHEN THE RINGS MEET - CLICKING EARLY COUNTS AS A MISS"),
		DetailFont,
		ESlateDrawEffect::None,
		PaperColor);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		BaseLayer + 2,
		AllottedGeometry.ToPaintGeometry(FVector2D(420.0f, 28.0f), FSlateLayoutTransform(FVector2D((ViewSize.X - 420.0f) * 0.5f, 112.0f))),
		FString::Printf(TEXT("HITS %d / %d     MISSES %d / %d"), Hits, RequiredHits, Misses, TargetCount - RequiredHits + 1),
		DetailFont,
		ESlateDrawEffect::None,
		PaperColor);

	if (bPepperAssisted)
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			BaseLayer + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(500.0f, 24.0f), FSlateLayoutTransform(FVector2D((ViewSize.X - 500.0f) * 0.5f, 142.0f))),
			TEXT("DISORIENTED - SLOWER TIMING"),
			DetailFont,
			ESlateDrawEffect::None,
			FLinearColor(0.72f, 0.48f, 0.95f, 1.0f));
	}

	if (!FeedbackText.IsEmpty())
	{
		const bool bPositive = FeedbackText == TEXT("HIT") || FeedbackText == TEXT("SECURED");
		FSlateDrawElement::MakeText(
			OutDrawElements,
			BaseLayer + 4,
			AllottedGeometry.ToPaintGeometry(FVector2D(360.0f, 40.0f), FSlateLayoutTransform(FVector2D((ViewSize.X - 360.0f) * 0.5f, ViewSize.Y - 100.0f))),
			FeedbackText,
			HeadingFont,
			ESlateDrawEffect::None,
			bPositive ? HitColor : MissColor);
	}

	if (bSessionActive && !bPendingCompletion && ActivePattern.IsValidIndex(ActiveTargetIndex))
	{
		const FVector2D Center = GetActiveTargetCenter(ViewSize);
		const float TargetRadius = GetScaledTargetRadius(ViewSize);
		const float ApproachDuration = GetApproachDurationForAssist(
			bPepperAssisted,
			NormalApproachDuration,
			PepperAssistedApproachDuration);
		const double Elapsed = FPlatformTime::Seconds() - CurrentTargetStartTime;
		const float ApproachAlpha = FMath::Clamp(static_cast<float>(Elapsed / ApproachDuration), 0.0f, 1.0f);
		const float ApproachRadius = FMath::Lerp(TargetRadius * 2.25f, TargetRadius, ApproachAlpha);

		DrawRing(AllottedGeometry, OutDrawElements, BaseLayer + 3, Center, ApproachRadius, FLinearColor(TargetColor.R, TargetColor.G, TargetColor.B, 0.65f), 3.0f);
		DrawRing(AllottedGeometry, OutDrawElements, BaseLayer + 3, Center, TargetRadius, TargetColor, 5.0f);
		DrawRing(AllottedGeometry, OutDrawElements, BaseLayer + 3, Center, TargetRadius * 0.72f, FLinearColor(PaperColor.R, PaperColor.G, PaperColor.B, 0.75f), 2.0f);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			BaseLayer + 4,
			AllottedGeometry.ToPaintGeometry(FVector2D(48.0f, 34.0f), FSlateLayoutTransform(Center - FVector2D(13.0f, 17.0f))),
			FString::FromInt(ActiveTargetIndex + 1),
			NumberFont,
			ESlateDrawEffect::None,
			PaperColor);
	}

	return BaseLayer + 5;
}

FReply UPPRestraintMinigameWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bSessionActive || bPendingCompletion || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
	}

	const double CurrentRealTime = FPlatformTime::Seconds();
	const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D TargetCenter = GetActiveTargetCenter(InGeometry.GetLocalSize());
	const float TargetRadius = GetScaledTargetRadius(InGeometry.GetLocalSize());
	const float ApproachDuration = GetApproachDurationForAssist(
		bPepperAssisted,
		NormalApproachDuration,
		PepperAssistedApproachDuration);
	const bool bInsideTarget = FVector2D::Distance(LocalPosition, TargetCenter) <= TargetRadius;
	const bool bValidTiming = IsHitTimingValid(
		CurrentRealTime - CurrentTargetStartTime,
		ApproachDuration,
		HitWindowDuration);

	if (bInsideTarget && bValidTiming)
	{
		RecordHit(CurrentRealTime);
	}
	else if (bInsideTarget)
	{
		RecordMiss(CurrentRealTime);
	}
	// Stray clicks outside the active target remain harmless.
	return FReply::Handled();
}

FReply UPPRestraintMinigameWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bSessionActive && !bPendingCompletion && InKeyEvent.GetKey() == EKeys::Escape)
	{
		QueueCompletion(EPPRestraintResult::Cancelled, FPlatformTime::Seconds());
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPPRestraintMinigameWidget::BuildPattern()
{
	const TArray<TArray<FVector2D>>& Patterns = GetRestraintPatterns();
	ActivePattern = Patterns[FMath::RandRange(0, Patterns.Num() - 1)];
	if (FMath::RandBool())
	{
		for (FVector2D& Point : ActivePattern)
		{
			Point.X = 1.0f - Point.X;
		}
	}
	TargetCount = FMath::Min(TargetCount, ActivePattern.Num());
	RequiredHits = FMath::Clamp(RequiredHits, 1, TargetCount);
}

void UPPRestraintMinigameWidget::RecordHit(double CurrentRealTime)
{
	++Hits;
	FeedbackText = TEXT("HIT");
	FeedbackExpiresAt = CurrentRealTime + HitFeedbackDuration;
	AdvanceOrFinish(CurrentRealTime);
}

void UPPRestraintMinigameWidget::RecordMiss(double CurrentRealTime)
{
	++Misses;
	FeedbackText = TEXT("MISS");
	FeedbackExpiresAt = CurrentRealTime + HitFeedbackDuration;
	AdvanceOrFinish(CurrentRealTime);
}

void UPPRestraintMinigameWidget::AdvanceOrFinish(double CurrentRealTime)
{
	++ActiveTargetIndex;
	if (Hits >= RequiredHits)
	{
		QueueCompletion(EPPRestraintResult::Success, CurrentRealTime);
		return;
	}
	if (!CanStillSucceed(Hits, ActiveTargetIndex, TargetCount, RequiredHits))
	{
		QueueCompletion(EPPRestraintResult::Failed, CurrentRealTime);
		return;
	}
	CurrentTargetStartTime = CurrentRealTime;
}

void UPPRestraintMinigameWidget::QueueCompletion(EPPRestraintResult Result, double CurrentRealTime)
{
	if (bPendingCompletion || !bSessionActive)
	{
		return;
	}
	bPendingCompletion = true;
	PendingResult = Result;
	FeedbackText = Result == EPPRestraintResult::Success ? TEXT("SECURED") : TEXT("BREAKAWAY");
	FeedbackExpiresAt = CurrentRealTime + ResultDisplayDuration;
	CompletionBroadcastTime = CurrentRealTime + ResultDisplayDuration;
}

FVector2D UPPRestraintMinigameWidget::GetActiveTargetCenter(const FVector2D& LocalSize) const
{
	if (!ActivePattern.IsValidIndex(ActiveTargetIndex))
	{
		return LocalSize * 0.5f;
	}
	return FVector2D(ActivePattern[ActiveTargetIndex].X * LocalSize.X, ActivePattern[ActiveTargetIndex].Y * LocalSize.Y);
}

float UPPRestraintMinigameWidget::GetScaledTargetRadius(const FVector2D& LocalSize) const
{
	const float Scale = FMath::Min(LocalSize.X / 1920.0f, LocalSize.Y / 1080.0f);
	return FMath::Max(56.0f, TargetRadiusAt1080p * Scale);
}

void UPPRestraintMinigameWidget::DrawRing(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FVector2D& Center,
	float Radius,
	const FLinearColor& Color,
	float Thickness) const
{
	constexpr int32 SegmentCount = 48;
	TArray<FVector2D> Points;
	Points.Reserve(SegmentCount + 1);
	for (int32 Segment = 0; Segment <= SegmentCount; ++Segment)
	{
		const float Angle = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(SegmentCount);
		Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
	}
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		Points,
		ESlateDrawEffect::None,
		Color,
		true,
		Thickness);
}
