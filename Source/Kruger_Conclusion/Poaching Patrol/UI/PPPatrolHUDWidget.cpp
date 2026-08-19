#include "PPPatrolHUDWidget.h"

#include "BaseGun.h"
#include "EnvironmentLevelSubsystem.h"
#include "Characters/ARangerCharacter.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Data/PPHealthComponent.h"
#include "Data/PPMinimapDefinition.h"
#include "Actors/PPArrestZone.h"
#include "Actors/PPRestPoint.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor SafariPanelColor(0.11f, 0.085f, 0.045f, 0.78f);
	const FLinearColor SafariInnerColor(0.18f, 0.145f, 0.075f, 0.42f);
	const FLinearColor SafariSoftPanelColor(0.11f, 0.085f, 0.045f, 0.52f);
	const FLinearColor SafariSoftInnerColor(0.18f, 0.145f, 0.075f, 0.28f);
	const FLinearColor SafariBorderColor(0.67f, 0.55f, 0.34f, 0.82f);
	const FLinearColor SafariTextColor(0.86f, 0.76f, 0.56f, 0.95f);
	const FLinearColor SafariMinorColor(0.62f, 0.53f, 0.38f, 0.58f);
	const FLinearColor SafariMarkerColor(0.96f, 0.62f, 0.22f, 1.0f);
	const FLinearColor SafariGreenColor(0.48f, 0.70f, 0.36f, 1.0f);
	const FLinearColor SafariDangerColor(0.92f, 0.22f, 0.14f, 1.0f);

	void DrawHudLine(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32& LayerId,
		const FVector2D& A,
		const FVector2D& B,
		const FLinearColor& Color,
		float Thickness)
	{
		TArray<FVector2D> Points;
		Points.Add(A);
		Points.Add(B);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
	}

	void DrawSafariPanel(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32& LayerId,
		const FSlateBrush* WhiteBrush,
		const FVector2D& Origin,
		const FVector2D& Size,
		bool bSoftPanel)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Origin)),
			WhiteBrush,
			ESlateDrawEffect::None,
			bSoftPanel ? SafariSoftPanelColor : SafariPanelColor);

		const FVector2D InnerOrigin = Origin + FVector2D(5.0f, 5.0f);
		const FVector2D InnerSize = Size - FVector2D(10.0f, 10.0f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(InnerSize, FSlateLayoutTransform(InnerOrigin)),
			WhiteBrush,
			ESlateDrawEffect::None,
			bSoftPanel ? SafariSoftInnerColor : SafariInnerColor);

		DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, Origin, Origin + FVector2D(Size.X, 0.0f), SafariBorderColor, 1.0f);
		DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, Origin + FVector2D(0.0f, Size.Y), Origin + Size, SafariBorderColor, 1.0f);
	}
}

UPPPatrolHUDWidget::UPPPatrolHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UPPPatrolHUDWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	DrawMinimap(AllottedGeometry, OutDrawElements, LayerId);
	DrawControlsHint(AllottedGeometry, OutDrawElements, LayerId);
	DrawCompass(AllottedGeometry, OutDrawElements, LayerId);
	DrawPlayerHealth(AllottedGeometry, OutDrawElements, LayerId);
	DrawToolCount(AllottedGeometry, OutDrawElements, LayerId);
	DrawObjectives(AllottedGeometry, OutDrawElements, LayerId);
	DrawEscortStatus(AllottedGeometry, OutDrawElements, LayerId);
	DrawCombatStatus(AllottedGeometry, OutDrawElements, LayerId);
	DrawRestPointPrompt(AllottedGeometry, OutDrawElements, LayerId);

	return LayerId;
}

void UPPPatrolHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	DamageFlashRemaining = FMath::Max(0.0f, DamageFlashRemaining - InDeltaTime);
	StatusRefreshAccumulator += InDeltaTime;
	if (StatusRefreshAccumulator < 0.1f)
	{
		return;
	}
	StatusRefreshAccumulator = 0.0f;
	RefreshMinimapActors();

	const APlayerController* PlayerController = GetOwningPlayer();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (const UPPHealthComponent* Health = PlayerPawn ? PlayerPawn->FindComponentByClass<UPPHealthComponent>() : nullptr)
	{
		const float CurrentHealth = Health->GetCurrentHealth();
		if (LastObservedPlayerHealth >= 0.0f && CurrentHealth < LastObservedPlayerHealth)
		{
			DamageFlashRemaining = 0.35f;
		}
		LastObservedPlayerHealth = CurrentHealth;
	}
	UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr;
	if (!LevelSubsystem)
	{
		CachedObjectives.Reset();
		bHasEscortStatus = false;
		CachedHostilePoacherCount = 0;
		CachedUrgentAttacker.Reset();
		return;
	}

	CachedObjectives = LevelSubsystem->GetObjectivesForPlayer(PlayerPawn);
	bHasEscortStatus = LevelSubsystem->GetMostUrgentEscortStatus(PlayerPawn, CachedEscortStatus);
	CachedHostilePoacherCount = 0;
	CachedUrgentAttacker.Reset();
	float BestAttackerDistanceSquared = TNumericLimits<float>::Max();
	for (APPPoacherCharacter* Poacher : LevelSubsystem->GetActivePoachers())
	{
		if (!IsValid(Poacher) || !Poacher->IsEngagingPlayer())
		{
			continue;
		}
		++CachedHostilePoacherCount;
		if (Poacher->IsPlayerAttackWindupActive() && PlayerPawn)
		{
			const float DistanceSquared = FVector::DistSquared(Poacher->GetActorLocation(), PlayerPawn->GetActorLocation());
			if (!CachedUrgentAttacker.IsValid() || DistanceSquared < BestAttackerDistanceSquared)
			{
				CachedUrgentAttacker = Poacher;
				BestAttackerDistanceSquared = DistanceSquared;
			}
		}
	}
}

void UPPPatrolHUDWidget::DrawCombatStatus(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	if (DamageFlashRemaining > 0.0f)
	{
		const float Alpha = 0.18f * FMath::Clamp(DamageFlashRemaining / 0.35f, 0.0f, 1.0f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(ViewSize, FSlateLayoutTransform()),
			WhiteBrush,
			ESlateDrawEffect::None,
			FLinearColor(0.85f, 0.02f, 0.01f, Alpha));
	}

	if (CachedHostilePoacherCount <= 0)
	{
		return;
	}

	const bool bIncomingAttack = CachedUrgentAttacker.IsValid();
	const FVector2D Size(300.0f, bIncomingAttack ? 64.0f : 38.0f);
	const FVector2D Origin((ViewSize.X - Size.X) * 0.5f, 86.0f);
	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, Origin, Size, true);
	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), bIncomingAttack ? 17 : 14);
	FLinearColor WarningColor = SafariDangerColor;
	if (bIncomingAttack && GetWorld())
	{
		WarningColor.A = 0.55f + 0.45f * FMath::Abs(FMath::Sin(GetWorld()->GetTimeSeconds() * 8.0f));
	}
	const FString Label = bIncomingAttack
		? TEXT("ATTACK INCOMING")
		: FString::Printf(TEXT("POACHERS ALERT: %d"), CachedHostilePoacherCount);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(Size.X - 24.0f, 24.0f), FSlateLayoutTransform(Origin + FVector2D(12.0f, 8.0f))),
		Label,
		LabelFont,
		ESlateDrawEffect::None,
		WarningColor);

	if (!bIncomingAttack)
	{
		return;
	}

	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	float DirectionOffset = 0.0f;
	if (PlayerPawn)
	{
		const FVector Delta = CachedUrgentAttacker->GetActorLocation() - PlayerPawn->GetActorLocation();
		const float Bearing = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		const float RelativeBearing = FRotator::NormalizeAxis(Bearing - PlayerPawn->GetControlRotation().Yaw);
		DirectionOffset = FMath::Clamp(RelativeBearing / 180.0f, -1.0f, 1.0f);
	}
	const float MarkerX = Origin.X + Size.X * 0.5f + DirectionOffset * (Size.X * 0.38f);
	FSlateDrawElement::MakeRotatedBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(12.0f, 12.0f), FSlateLayoutTransform(FVector2D(MarkerX - 6.0f, Origin.Y + 40.0f))),
		WhiteBrush,
		ESlateDrawEffect::None,
		UE_PI * 0.25f,
		FVector2D(6.0f, 6.0f),
		FSlateDrawElement::RelativeToElement,
		WarningColor);
}

void UPPPatrolHUDWidget::DrawRestPointPrompt(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn || !GetWorld())
	{
		return;
	}

	const APPRestPoint* RestPoint = nullptr;
	for (TActorIterator<APPRestPoint> It(GetWorld()); It; ++It)
	{
		if (*It && (*It)->IsRangerInRangeFor(PlayerPawn))
		{
			RestPoint = *It;
			break;
		}
	}
	if (!RestPoint)
	{
		return;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const FVector2D Size(320.0f, 58.0f);
	const FVector2D Origin((ViewSize.X - Size.X) * 0.5f, ViewSize.Y * 0.62f);
	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, Origin, Size, false);

	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(Size.X - 24.0f, 22.0f), FSlateLayoutTransform(Origin + FVector2D(12.0f, 8.0f))),
		RestPoint->GetInteractionPrompt().ToString(),
		LabelFont,
		ESlateDrawEffect::None,
		SafariTextColor);

	const FVector2D BarOrigin = Origin + FVector2D(12.0f, 36.0f);
	const FVector2D BarSize(Size.X - 24.0f, 10.0f);
	const float HoldProgress = RestPoint->GetHoldProgress();
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(BarSize, FSlateLayoutTransform(BarOrigin)),
		WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(0.08f, 0.06f, 0.035f, 0.8f));
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(BarSize.X * HoldProgress, BarSize.Y), FSlateLayoutTransform(BarOrigin)),
		WhiteBrush,
		ESlateDrawEffect::None,
		SafariGreenColor);
}

void UPPPatrolHUDWidget::DrawObjectives(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	if (CachedObjectives.IsEmpty())
	{
		return;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FVector2D Origin(24.0f, 76.0f);
	const FVector2D Size(368.0f, 126.0f);
	const FSlateFontInfo HeadingFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16);
	const FSlateFontInfo DetailFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12);
	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, Origin, Size, false);

	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(340.0f, 20.0f), FSlateLayoutTransform(Origin + FVector2D(14.0f, 10.0f))),
		TEXT("TODAY'S OBJECTIVES"), HeadingFont, ESlateDrawEffect::None, SafariMarkerColor);

	const FPPObjectiveState& Primary = CachedObjectives[0];
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(340.0f, 18.0f), FSlateLayoutTransform(Origin + FVector2D(14.0f, 38.0f))),
		Primary.Title.ToString(), DetailFont, ESlateDrawEffect::None, SafariTextColor);

	const float Progress = Primary.TargetValue > 0 ? FMath::Clamp(static_cast<float>(Primary.CurrentValue) / static_cast<float>(Primary.TargetValue), 0.0f, 1.0f) : 0.0f;
	const FVector2D BarOrigin = Origin + FVector2D(14.0f, 63.0f);
	const FVector2D BarSize(340.0f, 7.0f);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(BarSize, FSlateLayoutTransform(BarOrigin)), WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.08f, 0.06f, 0.035f, 0.8f));
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(BarSize.X * Progress, BarSize.Y), FSlateLayoutTransform(BarOrigin)), WhiteBrush, ESlateDrawEffect::None, SafariGreenColor);

	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(340.0f, 18.0f), FSlateLayoutTransform(Origin + FVector2D(14.0f, 78.0f))),
		Primary.Detail.ToString(), DetailFont, ESlateDrawEffect::None, SafariTextColor);

	for (const FPPObjectiveState& Objective : CachedObjectives)
	{
		if (Objective.Kind == EPPObjectiveKind::ConservationStatus)
		{
			FSlateDrawElement::MakeText(
				OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(340.0f, 18.0f), FSlateLayoutTransform(Origin + FVector2D(14.0f, 100.0f))),
				Objective.Title.ToString(), DetailFont, ESlateDrawEffect::None, SafariGreenColor);
			break;
		}
	}
}

void UPPPatrolHUDWidget::DrawEscortStatus(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	if (!bHasEscortStatus)
	{
		return;
	}

	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const FVector2D Size(440.0f, CachedEscortStatus.bUnderEscapePressure ? 116.0f : 86.0f);
	const FVector2D Origin((ViewSize.X - Size.X) * 0.5f, ViewSize.Y - Size.Y - 28.0f);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateFontInfo HeadingFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 15);
	const FSlateFontInfo DetailFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12);
	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, Origin, Size, false);

	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(410.0f, 20.0f), FSlateLayoutTransform(Origin + FVector2D(14.0f, 10.0f))),
		TEXT("ESCORT POACHER TO THE ARREST ZONE"), HeadingFont, ESlateDrawEffect::None, SafariMarkerColor);

	const FString AdditionalText = CachedEscortStatus.ActiveEscortCount > 1
		? FString::Printf(TEXT("  |  +%d additional escorts"), CachedEscortStatus.ActiveEscortCount - 1)
		: FString();
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(410.0f, 18.0f), FSlateLayoutTransform(Origin + FVector2D(14.0f, 38.0f))),
		FString::Printf(TEXT("Stay within %.0f m%s"), CachedEscortStatus.SafeRange / 100.0f, *AdditionalText), DetailFont, ESlateDrawEffect::None, SafariTextColor);

	if (!CachedEscortStatus.bUnderEscapePressure)
	{
		FSlateDrawElement::MakeText(
			OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(410.0f, 18.0f), FSlateLayoutTransform(Origin + FVector2D(14.0f, 59.0f))),
			TEXT("ESCORT SECURE"), DetailFont, ESlateDrawEffect::None, SafariGreenColor);
		return;
	}

	const float Risk = CachedEscortStatus.GetNormalizedEscapeProgress();
	FLinearColor RiskColor = Risk >= 0.7f ? SafariDangerColor : SafariMarkerColor;
	if (Risk >= 0.7f && GetWorld())
	{
		RiskColor.A = 0.55f + 0.45f * FMath::Abs(FMath::Sin(GetWorld()->GetTimeSeconds() * 5.0f));
	}
	const FVector2D BarOrigin = Origin + FVector2D(14.0f, 66.0f);
	const FVector2D BarSize(412.0f, 10.0f);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(BarSize, FSlateLayoutTransform(BarOrigin)), WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.08f, 0.06f, 0.035f, 0.8f));
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(BarSize.X * Risk, BarSize.Y), FSlateLayoutTransform(BarOrigin)), WhiteBrush, ESlateDrawEffect::None, RiskColor);
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(FVector2D(412.0f, 18.0f), FSlateLayoutTransform(Origin + FVector2D(14.0f, 84.0f))),
		FString::Printf(TEXT("ESCAPE RISK  •  %.1f seconds remaining"), CachedEscortStatus.SecondsRemaining), DetailFont, ESlateDrawEffect::None, RiskColor);
}

void UPPPatrolHUDWidget::DrawControlsHint(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const float ResponsiveMinimapSize = GetResponsiveMinimapSize(ViewSize);
	const FVector2D MapOrigin(
		MinimapBottomLeftOffset.X,
		ViewSize.Y - ResponsiveMinimapSize - MinimapBottomLeftOffset.Y);

	static const TCHAR* ControlLines[] = {
		TEXT("WASD to Move"),
		TEXT("E to Restrain Poacher"),
		TEXT("Space to jump"),
		TEXT("LClick: pepper spray slows restraint timing"),
		TEXT("Z / D-Pad Up to zoom map")
	};
	constexpr int32 LineCount = UE_ARRAY_COUNT(ControlLines);
	constexpr float LineHeight = 16.0f;
	constexpr float PanelPaddingX = 12.0f;
	constexpr float PanelPaddingY = 10.0f;
	constexpr float GapAboveMinimap = 8.0f;

	const FVector2D Size(
		FMath::Max(ResponsiveMinimapSize, 168.0f),
		PanelPaddingY * 2.0f + LineHeight * static_cast<float>(LineCount));
	const FVector2D Origin(MapOrigin.X, MapOrigin.Y - GapAboveMinimap - Size.Y);
	const FSlateFontInfo DetailFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11);

	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, Origin, Size, true);

	for (int32 Index = 0; Index < LineCount; ++Index)
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(Size.X - PanelPaddingX * 2.0f, LineHeight),
				FSlateLayoutTransform(Origin + FVector2D(PanelPaddingX, PanelPaddingY + LineHeight * static_cast<float>(Index)))),
			ControlLines[Index],
			DetailFont,
			ESlateDrawEffect::None,
			SafariTextColor);
	}
}

void UPPPatrolHUDWidget::DrawMinimap(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const float ResponsiveMinimapSize = GetResponsiveMinimapSize(ViewSize);
	const FVector2D MapSize(ResponsiveMinimapSize, ResponsiveMinimapSize);
	const FVector2D MapOrigin(
		MinimapBottomLeftOffset.X,
		ViewSize.Y - ResponsiveMinimapSize - MinimapBottomLeftOffset.Y);
	const FVector2D MapCenter = MapOrigin + (MapSize * 0.5f);
	const FVector2D InnerOrigin = MapOrigin + FVector2D(5.0f, 5.0f);
	const FVector2D InnerSize = MapSize - FVector2D(10.0f, 10.0f);
	const float MapHalfExtent = InnerSize.X * 0.46f;
	const float WorldRadius = GetCurrentMinimapWorldRadius();
	const float ViewYaw = PlayerPawn->GetControlRotation().Yaw;

	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, MapOrigin, MapSize, false);

	if (MinimapDefinition && MinimapDefinition->IsValidDefinition())
	{
		const FVector2D BoundsSize = MinimapDefinition->WorldBoundsMax - MinimapDefinition->WorldBoundsMin;
		const FVector2D PlayerXY(PlayerPawn->GetActorLocation().X, PlayerPawn->GetActorLocation().Y);
		const FVector2D CenterUV = MinimapDefinition->WorldToTextureUV(PlayerXY);
		const float BackgroundHalfWorldSize = WorldRadius * UE_SQRT_2;
		const FVector2D HalfUV(BackgroundHalfWorldSize / BoundsSize.X, BackgroundHalfWorldSize / BoundsSize.Y);

		FSlateBrush BackgroundBrush;
		BackgroundBrush.SetResourceObject(MinimapDefinition->BackgroundMaterial
			? static_cast<UObject*>(MinimapDefinition->BackgroundMaterial)
			: static_cast<UObject*>(MinimapDefinition->BackgroundTexture));
		BackgroundBrush.ImageSize = InnerSize * UE_SQRT_2;
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
		BackgroundBrush.SetUVRegion(FBox2f(
			FVector2f(CenterUV - HalfUV),
			FVector2f(CenterUV + HalfUV)));

		const FVector2D BackgroundSize = InnerSize * UE_SQRT_2;
		const FVector2D BackgroundOrigin = MapCenter - BackgroundSize * 0.5f;
		OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry.ToPaintGeometry(InnerSize, FSlateLayoutTransform(InnerOrigin))));
		FSlateDrawElement::MakeRotatedBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(BackgroundSize, FSlateLayoutTransform(BackgroundOrigin)),
			&BackgroundBrush,
			ESlateDrawEffect::None,
			FMath::DegreesToRadians(-90.0f - ViewYaw + MinimapDefinition->TextureRotationDegrees),
			BackgroundSize * 0.5f,
			FSlateDrawElement::RelativeToElement,
			FLinearColor(0.72f, 0.76f, 0.64f, 0.82f));
		OutDrawElements.PopClip();
	}

	const FVector2D NorthVector = ProjectWorldToMinimap(
		PlayerPawn->GetActorLocation() + FVector(WorldRadius, 0.0f, 0.0f),
		PlayerPawn->GetActorLocation(),
		ViewYaw,
		MapHalfExtent - 13.0f,
		WorldRadius);
	const FVector2D NorthLabelPosition = MapCenter + NorthVector - FVector2D(4.0f, 7.0f);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(12.0f, 14.0f), FSlateLayoutTransform(NorthLabelPosition)),
		TEXT("N"),
		FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10),
		ESlateDrawEffect::None,
		SafariTextColor);

	for (const TWeakObjectPtr<APPAnimalCharacter>& AnimalPtr : CachedMinimapAnimals)
	{
		const APPAnimalCharacter* Animal = AnimalPtr.Get();
		const UPPHealthComponent* Health = IsValid(Animal) ? Animal->FindComponentByClass<UPPHealthComponent>() : nullptr;
		if (!IsValid(Animal) || !Health || Health->IsDead())
		{
			continue;
		}
		const FVector2D LocalPoint = WorldToMinimap(Animal->GetActorLocation(), PlayerPawn, MapHalfExtent);
		if (FMath::Abs(LocalPoint.X) > MapHalfExtent || FMath::Abs(LocalPoint.Y) > MapHalfExtent)
		{
			continue;
		}
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(FVector2D(5.0f), FSlateLayoutTransform(MapCenter + LocalPoint - FVector2D(2.5f))),
			WhiteBrush,
			ESlateDrawEffect::None,
			AnimalColor);
	}

	for (const TWeakObjectPtr<APPPoacherCharacter>& PoacherPtr : CachedMinimapPoachers)
	{
		const APPPoacherCharacter* Poacher = PoacherPtr.Get();
		if (!IsValid(Poacher) || Poacher->GetPoacherState() == EPPPoacherState::Arrested || Poacher->GetPoacherState() == EPPPoacherState::Escaped)
		{
			continue;
		}
		const FVector2D LocalPoint = WorldToMinimap(Poacher->GetActorLocation(), PlayerPawn, MapHalfExtent);
		if (FMath::Abs(LocalPoint.X) > MapHalfExtent || FMath::Abs(LocalPoint.Y) > MapHalfExtent)
		{
			continue;
		}

		const bool bEscorted = Poacher->GetPoacherState() == EPPPoacherState::Captured || Poacher->GetPoacherState() == EPPPoacherState::FollowingPlayer;
		const float OuterSize = bEscorted ? 11.0f : 8.0f;
		if (bEscorted)
		{
			FLinearColor EscortColor = SafariMarkerColor;
			EscortColor.A = GetWorld() ? 0.55f + 0.35f * FMath::Abs(FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f)) : 0.8f;
			FSlateDrawElement::MakeRotatedBox(
				OutDrawElements, LayerId++,
				AllottedGeometry.ToPaintGeometry(FVector2D(OuterSize), FSlateLayoutTransform(MapCenter + LocalPoint - FVector2D(OuterSize * 0.5f))),
				WhiteBrush, ESlateDrawEffect::None, UE_PI * 0.25f, FVector2D(OuterSize * 0.5f),
				FSlateDrawElement::RelativeToElement, EscortColor);
		}
		FSlateDrawElement::MakeRotatedBox(
			OutDrawElements, LayerId++,
			AllottedGeometry.ToPaintGeometry(FVector2D(6.0f), FSlateLayoutTransform(MapCenter + LocalPoint - FVector2D(3.0f))),
			WhiteBrush, ESlateDrawEffect::None, UE_PI * 0.25f, FVector2D(3.0f),
			FSlateDrawElement::RelativeToElement, PoacherColor);
	}

	const APPArrestZone* NearestOutsideZone = nullptr;
	float NearestOutsideDistanceSquared = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<APPArrestZone>& ZonePtr : CachedArrestZones)
	{
		const APPArrestZone* Zone = ZonePtr.Get();
		if (!IsValid(Zone))
		{
			continue;
		}
		FVector2D LocalPoint = WorldToMinimap(Zone->GetActorLocation(), PlayerPawn, MapHalfExtent);
		const bool bInside = FMath::Abs(LocalPoint.X) <= MapHalfExtent && FMath::Abs(LocalPoint.Y) <= MapHalfExtent;
		if (!bInside)
		{
			const float DistanceSquared = FVector::DistSquared2D(Zone->GetActorLocation(), PlayerPawn->GetActorLocation());
			if (bHasEscortStatus && DistanceSquared < NearestOutsideDistanceSquared)
			{
				NearestOutsideZone = Zone;
				NearestOutsideDistanceSquared = DistanceSquared;
			}
			continue;
		}
		const FVector2D MarkerCenter = MapCenter + LocalPoint;
		DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, MarkerCenter + FVector2D(-5.0f, 0.0f), MarkerCenter + FVector2D(5.0f, 0.0f), ArrestZoneColor, 2.0f);
		DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, MarkerCenter + FVector2D(0.0f, -5.0f), MarkerCenter + FVector2D(0.0f, 5.0f), ArrestZoneColor, 2.0f);
	}

	if (NearestOutsideZone)
	{
		const FVector2D ClampedPoint = ClampMinimapPointToSquare(
			WorldToMinimap(NearestOutsideZone->GetActorLocation(), PlayerPawn, MapHalfExtent),
			MapHalfExtent - 7.0f);
		const FVector2D MarkerCenter = MapCenter + ClampedPoint;
		DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, MarkerCenter + FVector2D(-6.0f, 0.0f), MarkerCenter + FVector2D(6.0f, 0.0f), ArrestZoneColor, 2.5f);
		DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, MarkerCenter + FVector2D(0.0f, -6.0f), MarkerCenter + FVector2D(0.0f, 6.0f), ArrestZoneColor, 2.5f);
	}

	TArray<FVector2D> PlayerArrow = {
		MapCenter + FVector2D(0.0f, -10.0f),
		MapCenter + FVector2D(-6.0f, 7.0f),
		MapCenter,
		MapCenter + FVector2D(6.0f, 7.0f),
		MapCenter + FVector2D(0.0f, -10.0f)};
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), PlayerArrow, ESlateDrawEffect::None, PlayerColor, true, 2.5f);

	const FString RangeText = FString::Printf(TEXT("%.0f m"), WorldRadius / 100.0f);
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(48.0f, 14.0f), FSlateLayoutTransform(MapOrigin + FVector2D(10.0f, MapSize.Y - 21.0f))),
		RangeText, FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10), ESlateDrawEffect::None, SafariTextColor);
}

void UPPPatrolHUDWidget::DrawCompass(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerController || !PlayerPawn)
	{
		return;
	}

	const float Yaw = FRotator::ClampAxis(PlayerController->GetControlRotation().Yaw);
	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const float LocalCompassWidth = FMath::Min(CompassWidth, FMath::Max(260.0f, ViewSize.X - 48.0f));
	const FVector2D Start((ViewSize.X - LocalCompassWidth) * 0.5f, CompassTopOffset);
	const FVector2D CompassSize(LocalCompassWidth, 34.0f);
	const FVector2D Center(Start.X + LocalCompassWidth * 0.5f, Start.Y + 17.0f);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateFontInfo CardinalFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12);
	const FSlateFontInfo IntercardinalFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10);
	const float VisibleDegrees = 110.0f;
	const float HalfWidth = LocalCompassWidth * 0.5f;

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(CompassSize, FSlateLayoutTransform(Start)),
		WhiteBrush,
		ESlateDrawEffect::None,
		SafariPanelColor);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(LocalCompassWidth - 10.0f, 18.0f), FSlateLayoutTransform(Start + FVector2D(5.0f, 8.0f))),
		WhiteBrush,
		ESlateDrawEffect::None,
		SafariInnerColor);

	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, Start, Start + FVector2D(LocalCompassWidth, 0.0f), SafariBorderColor, 1.0f);
	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, Start + FVector2D(0.0f, CompassSize.Y), Start + CompassSize, SafariBorderColor, 1.0f);
	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, Start + FVector2D(6.0f, Center.Y - Start.Y), Start + FVector2D(LocalCompassWidth - 6.0f, Center.Y - Start.Y), SafariMinorColor, 1.0f);

	for (int32 Degrees = 0; Degrees < 360; Degrees += 15)
	{
		const float Delta = FRotator::NormalizeAxis(static_cast<float>(Degrees) - Yaw);
		if (FMath::Abs(Delta) > VisibleDegrees)
		{
			continue;
		}

		const float X = Center.X + (Delta / VisibleDegrees) * HalfWidth;
		if (X < Start.X + 10.0f || X > Start.X + LocalCompassWidth - 10.0f)
		{
			continue;
		}

		const bool bCardinal = Degrees % 90 == 0;
		const bool bIntercardinal = Degrees % 45 == 0;
		const float TickHeight = bCardinal ? 10.0f : (bIntercardinal ? 8.0f : 5.0f);
		const float TickThickness = bCardinal ? 1.6f : 1.0f;
		DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, FVector2D(X, Center.Y - TickHeight * 0.5f), FVector2D(X, Center.Y + TickHeight * 0.5f), bIntercardinal ? SafariTextColor : SafariMinorColor, TickThickness);

		const TCHAR* Label = nullptr;
		switch (Degrees)
		{
		case 0:
			Label = TEXT("N");
			break;
		case 45:
			Label = TEXT("NE");
			break;
		case 90:
			Label = TEXT("E");
			break;
		case 135:
			Label = TEXT("SE");
			break;
		case 180:
			Label = TEXT("S");
			break;
		case 225:
			Label = TEXT("SW");
			break;
		case 270:
			Label = TEXT("W");
			break;
		case 315:
			Label = TEXT("NW");
			break;
		default:
			break;
		}

		if (!Label)
		{
			continue;
		}

		const FVector2D TextSize(bCardinal ? 18.0f : 24.0f, 14.0f);
		const FSlateFontInfo& Font = bCardinal ? CardinalFont : IntercardinalFont;
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(FVector2D(X - TextSize.X * 0.5f, Start.Y + 3.0f))),
			FString(Label),
			Font,
			ESlateDrawEffect::None,
			bCardinal ? SafariMarkerColor : SafariTextColor);
	}

	for (const TWeakObjectPtr<APPPoacherCharacter>& PoacherPtr : CachedMinimapPoachers)
	{
		const APPPoacherCharacter* Poacher = PoacherPtr.Get();
		if (!IsValid(Poacher)
			|| Poacher->GetPoacherState() == EPPPoacherState::Arrested
			|| Poacher->GetPoacherState() == EPPPoacherState::Escaped)
		{
			continue;
		}

		float NormalizedOffset = 0.0f;
		if (!ProjectWorldToCompass(
			Poacher->GetActorLocation(),
			PlayerPawn->GetActorLocation(),
			Yaw,
			VisibleDegrees,
			CompassPoacherRange,
			NormalizedOffset))
		{
			continue;
		}

		const bool bEscorted = Poacher->GetPoacherState() == EPPPoacherState::Captured
			|| Poacher->GetPoacherState() == EPPPoacherState::FollowingPlayer;
		FLinearColor MarkerColor = bEscorted ? EscortedPoacherColor : PoacherColor;
		if (bEscorted && GetWorld())
		{
			MarkerColor.A *= 0.7f + 0.3f * FMath::Abs(FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f));
		}

		const float MarkerSize = FMath::Clamp(CompassPoacherMarkerSize, 2.0f, 24.0f);
		const FVector2D MarkerExtent(MarkerSize, MarkerSize);
		const FVector2D MarkerOrigin(
			Center.X + NormalizedOffset * HalfWidth - MarkerSize * 0.5f,
			Start.Y + CompassSize.Y - MarkerSize - 2.0f);
		FSlateDrawElement::MakeRotatedBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(MarkerExtent, FSlateLayoutTransform(MarkerOrigin)),
			WhiteBrush,
			ESlateDrawEffect::None,
			UE_PI * 0.25f,
			FVector2D(MarkerSize * 0.5f),
			FSlateDrawElement::RelativeToElement,
			MarkerColor);
	}

	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, Center + FVector2D(0.0f, -13.0f), Center + FVector2D(0.0f, 13.0f), SafariMarkerColor, 2.0f);
	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, Center + FVector2D(-7.0f, -12.0f), Center + FVector2D(0.0f, -5.0f), SafariMarkerColor, 1.4f);
	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, Center + FVector2D(7.0f, -12.0f), Center + FVector2D(0.0f, -5.0f), SafariMarkerColor, 1.4f);
}

void UPPPatrolHUDWidget::DrawPlayerHealth(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UPPHealthComponent* HealthComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UPPHealthComponent>() : nullptr;
	if (!HealthComponent)
	{
		return;
	}

	const float MaxHealth = FMath::Max(1.0f, HealthComponent->GetMaxHealth());
	const float CurrentHealth = FMath::Clamp(HealthComponent->GetCurrentHealth(), 0.0f, MaxHealth);
	const float HealthPercent = CurrentHealth / MaxHealth;

	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const FVector2D Size(176.0f, 28.0f);
	const FVector2D Origin(ViewSize.X - Size.X - HealthOffset.X, ViewSize.Y - Size.Y - HealthOffset.Y);
	const FVector2D BarOrigin = Origin + FVector2D(12.0f, 10.0f);
	const FVector2D BarSize(Size.X - 24.0f, 8.0f);
	const FVector2D ThumbSize(4.0f, 18.0f);
	const float ThumbX = BarOrigin.X + (BarSize.X * HealthPercent) - (ThumbSize.X * 0.5f);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11);
	const FLinearColor HealthColor = HealthPercent <= 0.3f
		? SafariDangerColor
		: SafariGreenColor;

	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, Origin, Size, true);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(76.0f, 14.0f), FSlateLayoutTransform(Origin + FVector2D(12.0f, -5.0f))),
		TEXT("HEALTH"),
		LabelFont,
		ESlateDrawEffect::None,
		SafariTextColor);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(BarSize, FSlateLayoutTransform(BarOrigin)),
		WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(0.09f, 0.075f, 0.045f, 0.58f));

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(BarSize.X * HealthPercent, BarSize.Y), FSlateLayoutTransform(BarOrigin)),
		WhiteBrush,
		ESlateDrawEffect::None,
		HealthColor);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(ThumbSize, FSlateLayoutTransform(FVector2D(ThumbX, BarOrigin.Y - 5.0f))),
		WhiteBrush,
		ESlateDrawEffect::None,
		SafariTextColor);
}

void UPPPatrolHUDWidget::DrawToolCount(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	ABaseGun* Tool = FindCurrentTool();
	if (!Tool)
	{
		return;
	}

	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const FVector2D Size(176.0f, 42.0f);
	const FVector2D Origin(ViewSize.X - Size.X - ToolCountOffset.X, ViewSize.Y - Size.Y - ToolCountOffset.Y);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11);
	const FSlateFontInfo CountFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 19);

	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, Origin, Size, true);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(Size.X - 24.0f, 14.0f), FSlateLayoutTransform(Origin + FVector2D(12.0f, 5.0f))),
		Tool->GetToolDisplayName().ToString().ToUpper(),
		LabelFont,
		ESlateDrawEffect::None,
		SafariTextColor);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(Size.X - 24.0f, 22.0f), FSlateLayoutTransform(Origin + FVector2D(12.0f, 18.0f))),
		FString::Printf(TEXT("%d / %d"), Tool->GetRemainingUses(), Tool->GetMaxUses()),
		CountFont,
		ESlateDrawEffect::None,
		SafariMarkerColor);
}

ABaseGun* UPPPatrolHUDWidget::FindCurrentTool() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const ARangerCharacter* Ranger = Cast<ARangerCharacter>(Pawn);
	if (Ranger && Ranger->GetCurrentGun())
	{
		return Ranger->GetCurrentGun();
	}

	for (TActorIterator<ABaseGun> It(GetWorld()); It; ++It)
	{
		ABaseGun* Tool = *It;
		if (Tool && Tool->GetOwner() == Pawn)
		{
			return Tool;
		}
	}

	return nullptr;
}

void UPPPatrolHUDWidget::CycleMinimapZoom()
{
	if (MinimapZoomRadii.IsEmpty())
	{
		CurrentMinimapZoomIndex = 0;
		return;
	}
	CurrentMinimapZoomIndex = (CurrentMinimapZoomIndex + 1) % MinimapZoomRadii.Num();
}

float UPPPatrolHUDWidget::GetCurrentMinimapWorldRadius() const
{
	if (MinimapZoomRadii.IsEmpty())
	{
		return 8000.0f;
	}
	return FMath::Max(1.0f, MinimapZoomRadii[FMath::Clamp(CurrentMinimapZoomIndex, 0, MinimapZoomRadii.Num() - 1)]);
}

FVector2D UPPPatrolHUDWidget::ProjectWorldToMinimap(
	const FVector& WorldLocation,
	const FVector& PlayerLocation,
	float ViewYawDegrees,
	float MapRadius,
	float WorldRadius)
{
	const FVector Delta = WorldLocation - PlayerLocation;
	const float Scale = MapRadius / FMath::Max(1.0f, WorldRadius);
	const FRotator ViewYaw(0.0f, ViewYawDegrees, 0.0f);
	const FVector Forward = FRotationMatrix(ViewYaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(ViewYaw).GetUnitAxis(EAxis::Y);

	return FVector2D(
		FVector::DotProduct(Delta, Right),
		-FVector::DotProduct(Delta, Forward)) * Scale;
}

FVector2D UPPPatrolHUDWidget::ClampMinimapPointToSquare(const FVector2D& Point, float HalfExtent)
{
	const float LargestAxis = FMath::Max(FMath::Abs(Point.X), FMath::Abs(Point.Y));
	return LargestAxis > HalfExtent && LargestAxis > KINDA_SMALL_NUMBER
		? Point * (HalfExtent / LargestAxis)
		: Point;
}

bool UPPPatrolHUDWidget::ProjectWorldToCompass(
	const FVector& WorldLocation,
	const FVector& PlayerLocation,
	float ViewYawDegrees,
	float VisibleDegrees,
	float MaxWorldDistance,
	float& OutNormalizedOffset)
{
	OutNormalizedOffset = 0.0f;
	const FVector2D Delta(WorldLocation.X - PlayerLocation.X, WorldLocation.Y - PlayerLocation.Y);
	const float SafeVisibleDegrees = FMath::Max(1.0f, VisibleDegrees);
	const float SafeMaxDistance = FMath::Max(0.0f, MaxWorldDistance);
	if (Delta.SizeSquared() > FMath::Square(SafeMaxDistance))
	{
		return false;
	}

	const float WorldBearing = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	const float RelativeBearing = FRotator::NormalizeAxis(WorldBearing - ViewYawDegrees);
	if (FMath::Abs(RelativeBearing) > SafeVisibleDegrees)
	{
		return false;
	}

	OutNormalizedOffset = RelativeBearing / SafeVisibleDegrees;
	return true;
}

float UPPPatrolHUDWidget::GetResponsiveMinimapSize(const FVector2D& ViewSize) const
{
	return FMath::Clamp(MinimapSize * (ViewSize.Y / 1080.0f), 288.0f, 396.0f);
}

FVector2D UPPPatrolHUDWidget::WorldToMinimap(const FVector& WorldLocation, const APawn* PlayerPawn, float MapRadius) const
{
	return PlayerPawn
		? ProjectWorldToMinimap(WorldLocation, PlayerPawn->GetActorLocation(), PlayerPawn->GetControlRotation().Yaw, MapRadius, GetCurrentMinimapWorldRadius())
		: FVector2D::ZeroVector;
}

void UPPPatrolHUDWidget::RefreshMinimapActors()
{
	CachedMinimapPoachers.Reset();
	CachedMinimapAnimals.Reset();
	CachedArrestZones.Reset();

	if (const UEnvironmentLevelSubsystem* LevelSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEnvironmentLevelSubsystem>() : nullptr)
	{
		for (APPPoacherCharacter* Poacher : LevelSubsystem->GetActivePoachers())
		{
			CachedMinimapPoachers.Add(Poacher);
		}
		for (APPAnimalCharacter* Animal : LevelSubsystem->GetLivingAnimals())
		{
			CachedMinimapAnimals.Add(Animal);
		}
	}

	if (GetWorld())
	{
		for (TActorIterator<APPArrestZone> It(GetWorld()); It; ++It)
		{
			CachedArrestZones.Add(*It);
		}
	}
}
