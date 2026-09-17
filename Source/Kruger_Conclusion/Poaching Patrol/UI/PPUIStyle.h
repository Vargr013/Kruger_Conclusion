#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace PPUIStyle
{
    inline const FLinearColor Ink(0.015f, 0.012f, 0.008f, 1.0f);

    inline FSlateFontInfo Font(FName Typeface, int32 Size)
    {
        FSlateFontInfo Result = FCoreStyle::GetDefaultFontStyle(Typeface, Size);
        Result.OutlineSettings.OutlineSize = 1;
        Result.OutlineSettings.OutlineColor = Ink;
        return Result;
    }

    inline void DrawOutline(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32& Layer,
        const FVector2D& Origin, const FVector2D& Size)
    {
        const FVector2D Start = Origin + FVector2D(1.5f);
        const FVector2D End = Origin + Size - FVector2D(1.5f);
        const TArray<FVector2D> Corners = {Start, FVector2D(End.X, Start.Y), End, FVector2D(Start.X, End.Y), Start};
        FSlateDrawElement::MakeLines(Elements, Layer++, Geometry.ToPaintGeometry(), Corners,
            ESlateDrawEffect::None, Ink, true, 3.0f);
    }

    inline FSlateBrush PanelBrush(const FLinearColor& Fill)
    {
        return FSlateRoundedBoxBrush(Fill, 0.0f, Ink, 3.0f);
    }

    inline void OutlineButton(UButton* Button)
    {
        FButtonStyle Style = Button->GetStyle();
        Style.Normal = PanelBrush(FLinearColor(0.18f, 0.145f, 0.075f, 1.0f));
        Style.Hovered = PanelBrush(FLinearColor(0.32f, 0.26f, 0.14f, 1.0f));
        Style.Pressed = PanelBrush(FLinearColor(0.09f, 0.07f, 0.035f, 1.0f));
        Style.Disabled = PanelBrush(FLinearColor(0.10f, 0.10f, 0.09f, 1.0f));
        Button->SetStyle(Style);
    }
}
