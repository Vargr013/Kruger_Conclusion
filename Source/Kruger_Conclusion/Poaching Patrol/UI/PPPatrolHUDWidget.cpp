#include "PPPatrolHUDWidget.h"

#include "BaseGun.h"
#include "Characters/ARangerCharacter.h"
#include "Characters/PPAnimalCharacter.h"
#include "Characters/PPPoacherCharacter.h"
#include "Actors/PPArrestZone.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

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

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(MapSize, FSlateLayoutTransform(MapOrigin)),
		WhiteBrush,
		ESlateDrawEffect::None,
		PanelColor);

	TArray<FVector2D> NorthLine;
	NorthLine.Add(MapCenter);
	NorthLine.Add(MapCenter + FVector2D(0.0f, -MapSize.Y * 0.42f));
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), NorthLine, ESlateDrawEffect::None, LineColor, true, 1.0f);

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
		PlayerColor);
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
	const FVector2D Start((ViewSize.X - CompassWidth) * 0.5f, CompassTopOffset);
	const FVector2D Center(Start.X + CompassWidth * 0.5f, Start.Y + 12.0f);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(CompassWidth, 24.0f), FSlateLayoutTransform(Start)),
		WhiteBrush,
		ESlateDrawEffect::None,
		PanelColor);

	TArray<FVector2D> CenterTick;
	CenterTick.Add(Center + FVector2D(0.0f, -8.0f));
	CenterTick.Add(Center + FVector2D(0.0f, 8.0f));
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), CenterTick, ESlateDrawEffect::None, PlayerColor, true, 2.0f);

	struct FCompassMark
	{
		float Degrees;
		const TCHAR* Label;
	};

	const FCompassMark Marks[] = {
		{ 0.0f, TEXT("N") },
		{ 90.0f, TEXT("E") },
		{ 180.0f, TEXT("S") },
		{ 270.0f, TEXT("W") }
	};

	for (const FCompassMark& Mark : Marks)
	{
		float Delta = FRotator::NormalizeAxis(Mark.Degrees - Yaw);
		const float X = Center.X + (Delta / 90.0f) * (CompassWidth * 0.5f);
		if (X < Start.X + 8.0f || X > Start.X + CompassWidth - 18.0f)
		{
			continue;
		}

		TArray<FVector2D> Tick;
		Tick.Add(FVector2D(X, Center.Y - 5.0f));
		Tick.Add(FVector2D(X, Center.Y + 5.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), Tick, ESlateDrawEffect::None, LineColor, true, 1.0f);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId++,
			AllottedGeometry.ToPaintGeometry(FVector2D(20.0f, 16.0f), FSlateLayoutTransform(FVector2D(X - 4.0f, Center.Y - 8.0f))),
			FString(Mark.Label),
			Font,
			ESlateDrawEffect::None,
			LineColor);
	}
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

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Origin)),
		WhiteBrush,
		ESlateDrawEffect::None,
		PanelColor);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(Size.X - 24.0f, 14.0f), FSlateLayoutTransform(Origin + FVector2D(12.0f, 5.0f))),
		Tool->GetToolDisplayName().ToString().ToUpper(),
		LabelFont,
		ESlateDrawEffect::None,
		LineColor);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(FVector2D(Size.X - 24.0f, 22.0f), FSlateLayoutTransform(Origin + FVector2D(12.0f, 18.0f))),
		FString::Printf(TEXT("%d / %d"), Tool->GetRemainingUses(), Tool->GetMaxUses()),
		CountFont,
		ESlateDrawEffect::None,
		PlayerColor);
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
