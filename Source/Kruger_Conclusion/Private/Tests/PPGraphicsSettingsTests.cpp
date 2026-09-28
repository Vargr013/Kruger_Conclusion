#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/PPGameUserSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPGraphicsPresetMappingsTest,
	"KrugerConclusion.PoachingPatrol.Graphics.PresetMappings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPGraphicsPresetMappingsTest::RunTest(const FString& Parameters)
{
	const Scalability::FQualityLevels Low = UPPGameUserSettings::BuildQualityLevels(EPPGraphicsPreset::Low);
	const Scalability::FQualityLevels Medium = UPPGameUserSettings::BuildQualityLevels(EPPGraphicsPreset::Medium);
	const Scalability::FQualityLevels High = UPPGameUserSettings::BuildQualityLevels(EPPGraphicsPreset::High);
	TestEqual(TEXT("Low uses quality group 1"), Low.ViewDistanceQuality, 1);
	TestEqual(TEXT("Low uses 50 percent TSR"), Low.ResolutionQuality, 50.0f);
	TestEqual(TEXT("Medium uses quality group 2"), Medium.ViewDistanceQuality, 2);
	TestTrue(TEXT("Medium uses 66.7 percent TSR"), FMath::IsNearlyEqual(Medium.ResolutionQuality, 66.6667f, 0.01f));
	TestEqual(TEXT("High uses quality group 3"), High.ViewDistanceQuality, 3);
	TestEqual(TEXT("High uses native resolution"), High.ResolutionQuality, 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPPGraphicsPresetDefaultsTest,
	"KrugerConclusion.PoachingPatrol.Graphics.DefaultAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPPGraphicsPresetDefaultsTest::RunTest(const FString& Parameters)
{
	UPPGameUserSettings* Settings = NewObject<UPPGameUserSettings>();
	Settings->SetToDefaults();
	TestEqual(TEXT("New installs default to Medium"), Settings->GetGraphicsPreset(), EPPGraphicsPreset::Medium);
	TestEqual(TEXT("Invalid low saved value falls back to Medium"), UPPGameUserSettings::SanitizePreset(-1), EPPGraphicsPreset::Medium);
	TestEqual(TEXT("Invalid high saved value falls back to Medium"), UPPGameUserSettings::SanitizePreset(99), EPPGraphicsPreset::Medium);
	TestEqual(TEXT("Mouse sensitivity defaults to 1.0"), Settings->GetMouseSensitivity(), UPPGameUserSettings::DefaultMouseSensitivity);
	TestEqual(TEXT("Mouse sensitivity clamps low values"), UPPGameUserSettings::SanitizeMouseSensitivity(0.01f), UPPGameUserSettings::MinMouseSensitivity);
	TestEqual(TEXT("Mouse sensitivity clamps high values"), UPPGameUserSettings::SanitizeMouseSensitivity(5.0f), UPPGameUserSettings::MaxMouseSensitivity);
	return true;
}

#endif
