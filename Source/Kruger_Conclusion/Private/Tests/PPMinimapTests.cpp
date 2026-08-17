#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/PPMinimapDefinition.h"
#include "Engine/Texture2D.h"
#include "UI/PPPatrolHUDWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPMinimapProjectionTest,
	"KrugerConclusion.PoachingPatrol.Minimap.ProjectionAndClamping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPMinimapProjectionTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	const float WorldRadius = 1000.0f;
	const float MapRadius = 100.0f;

	auto TestProjection = [this, &Origin, WorldRadius, MapRadius](const TCHAR* Label, const FVector& Location, float Yaw, const FVector2D& Expected)
	{
		const FVector2D Actual = UPPPatrolHUDWidget::ProjectWorldToMinimap(Location, Origin, Yaw, MapRadius, WorldRadius);
		TestTrue(Label, Actual.Equals(Expected, 0.01f));
	};

	TestProjection(TEXT("Yaw 0 projects north upward"), FVector(1000.0f, 0.0f, 0.0f), 0.0f, FVector2D(0.0f, -100.0f));
	TestProjection(TEXT("Yaw 90 projects north left"), FVector(1000.0f, 0.0f, 0.0f), 90.0f, FVector2D(-100.0f, 0.0f));
	TestProjection(TEXT("Yaw 180 projects north down"), FVector(1000.0f, 0.0f, 0.0f), 180.0f, FVector2D(0.0f, 100.0f));
	TestProjection(TEXT("Yaw 270 projects north right"), FVector(1000.0f, 0.0f, 0.0f), 270.0f, FVector2D(100.0f, 0.0f));
	TestProjection(TEXT("World right projects screen right"), FVector(0.0f, 1000.0f, 0.0f), 0.0f, FVector2D(100.0f, 0.0f));

	for (const float ZoomRadius : {4000.0f, 8000.0f, 14000.0f})
	{
		const FVector2D AtRangeEdge = UPPPatrolHUDWidget::ProjectWorldToMinimap(
			FVector(ZoomRadius, 0.0f, 0.0f), Origin, 0.0f, MapRadius, ZoomRadius);
		TestTrue(
			*FString::Printf(TEXT("%.0f cm zoom radius projects to the same map edge"), ZoomRadius),
			AtRangeEdge.Equals(FVector2D(0.0f, -MapRadius), 0.01f));
	}

	TestTrue(
		TEXT("Outside point clamps to square edge without changing direction"),
		UPPPatrolHUDWidget::ClampMinimapPointToSquare(FVector2D(200.0f, -100.0f), 100.0f).Equals(FVector2D(100.0f, -50.0f), 0.01f));
	TestTrue(
		TEXT("Inside point is unchanged"),
		UPPPatrolHUDWidget::ClampMinimapPointToSquare(FVector2D(40.0f, -20.0f), 100.0f).Equals(FVector2D(40.0f, -20.0f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPMinimapZoomAndDefinitionTest,
	"KrugerConclusion.PoachingPatrol.Minimap.ZoomAndDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPMinimapZoomAndDefinitionTest::RunTest(const FString& Parameters)
{
	UPPPatrolHUDWidget* Widget = NewObject<UPPPatrolHUDWidget>();
	if (!TestNotNull(TEXT("Minimap widget can be created"), Widget))
	{
		return false;
	}

	TestEqual(TEXT("Medium range is the default"), Widget->GetCurrentMinimapWorldRadius(), 8000.0f);
	Widget->CycleMinimapZoom();
	TestEqual(TEXT("First cycle selects far range"), Widget->GetCurrentMinimapWorldRadius(), 14000.0f);
	Widget->CycleMinimapZoom();
	TestEqual(TEXT("Second cycle wraps to near range"), Widget->GetCurrentMinimapWorldRadius(), 4000.0f);
	Widget->CycleMinimapZoom();
	TestEqual(TEXT("Third cycle returns to medium range"), Widget->GetCurrentMinimapWorldRadius(), 8000.0f);

	UPPMinimapDefinition* Definition = NewObject<UPPMinimapDefinition>();
	TestFalse(TEXT("Definition without a texture uses the fallback background"), Definition->IsValidDefinition());
	Definition->BackgroundTexture = NewObject<UTexture2D>();
	Definition->WorldBoundsMin = FVector2D(0.0f, 0.0f);
	Definition->WorldBoundsMax = FVector2D(100.0f, 100.0f);
	TestTrue(TEXT("Texture and positive bounds produce a valid definition"), Definition->IsValidDefinition());
	TestTrue(TEXT("World location inside the definition is mapped"), Definition->IsWorldLocationMapped(FVector2D(50.0f, 50.0f)));
	TestTrue(TEXT("Map boundary remains part of the mapped region"), Definition->IsWorldLocationMapped(FVector2D(100.0f, 100.0f)));
	TestFalse(TEXT("World location beyond map bounds uses material fallback"), Definition->IsWorldLocationMapped(FVector2D(100.01f, 50.0f)));
	TestTrue(TEXT("World center converts to centered UVs"), Definition->WorldToTextureUV(FVector2D(50.0f, 50.0f)).Equals(FVector2D(0.5f, 0.5f), 0.001f));
	Definition->WorldBoundsMax = Definition->WorldBoundsMin;
	TestFalse(TEXT("Degenerate map bounds use the fallback background"), Definition->IsValidDefinition());
	return true;
}

#endif
