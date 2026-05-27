#include "PPPatrolHUDWidget.h"

#include "BaseGun.h"
#include "Characters/ARangerCharacter.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Data/PPHealthComponent.h"
#include "Actors/PPArrestZone.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/PlayerController.h"
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
	DrawCompass(AllottedGeometry, OutDrawElements, LayerId);
	DrawPlayerHealth(AllottedGeometry, OutDrawElements, LayerId);
	DrawToolCount(AllottedGeometry, OutDrawElements, LayerId);

	return LayerId;
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
	const FVector2D MapSize(MinimapSize, MinimapSize);
	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const FVector2D MapOrigin(
		MinimapBottomLeftOffset.X,
		ViewSize.Y - MinimapSize - MinimapBottomLeftOffset.Y);
	const FVector2D MapCenter = MapOrigin + (MapSize * 0.5f);
	const float DotSize = 5.0f;

	DrawSafariPanel(AllottedGeometry, OutDrawElements, LayerId, WhiteBrush, MapOrigin, MapSize, false);

	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, MapCenter, MapCenter + FVector2D(0.0f, -MapSize.Y * 0.42f), SafariTextColor, 1.0f);
	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, MapCenter + FVector2D(-MapSize.X * 0.34f, 0.0f), MapCenter + FVector2D(MapSize.X * 0.34f, 0.0f), SafariMinorColor, 1.0f);
	DrawHudLine(AllottedGeometry, OutDrawElements, LayerId, MapCenter + FVector2D(0.0f, MapSize.Y * 0.34f), MapCenter + FVector2D(0.0f, -MapSize.Y * 0.34f), SafariMinorColor, 1.0f);

	int32 DrawnActors = 0;
	for (TActorIterator<AActor> It(GetWorld()); It && DrawnActors < MaxMinimapActors; ++It)
	{
		const AActor* Actor = *It;
		FLinearColor DotColor;
		float LocalDotSize = DotSize;

		if (Actor->IsA<APPPoacherCharacter>())
		{
			DotColor = PoacherColor;
		}
		else if (Actor->IsA<APPAnimalCharacter>())
		{
			DotColor = AnimalColor;
		}
		else if (Actor->IsA<APPArrestZone>())
		{
			DotColor = ArrestZoneColor;
			LocalDotSize = 7.0f;
		}
		else
		{
			continue;
		}

		const FVector2D LocalPoint = WorldToMinimap(Actor->GetActorLocation(), PlayerPawn, MinimapSize * 0.44f);
		if (FMath::Abs(LocalPoint.X) > MinimapSize * 0.44f || FMath::Abs(LocalPoint.Y) > MinimapSize * 0.44f)
		{
			continue;
		}

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(FVector2D(LocalDotSize), FSlateLayoutTransform(MapCenter + LocalPoint - FVector2D(LocalDotSize * 0.5f))),
			WhiteBrush,
			ESlateDrawEffect::None,
			DotColor);
		DrawnActors++;
	}

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(8.0f), FSlateLayoutTransform(MapCenter - FVector2D(4.0f))),
		WhiteBrush,
		ESlateDrawEffect::None,
		SafariGreenColor);
}

void UPPPatrolHUDWidget::DrawCompass(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32& LayerId) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
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

FVector2D UPPPatrolHUDWidget::WorldToMinimap(const FVector& WorldLocation, const APawn* PlayerPawn, float MapRadius) const
{
	const FVector Delta = WorldLocation - PlayerPawn->GetActorLocation();
	const float Scale = MapRadius / FMath::Max(1.0f, MinimapWorldRadius);
	const FRotator ViewYaw(0.0f, PlayerPawn->GetControlRotation().Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(ViewYaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(ViewYaw).GetUnitAxis(EAxis::Y);

	return FVector2D(
		FVector::DotProduct(Delta, Right),
		-FVector::DotProduct(Delta, Forward)) * Scale;
}
