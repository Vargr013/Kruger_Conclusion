#include "Data/PPGameUserSettings.h"

#include "Engine/Engine.h"

UPPGameUserSettings::UPPGameUserSettings()
{
	SelectedGraphicsPreset = EPPGraphicsPreset::Medium;
	MouseSensitivity = DefaultMouseSensitivity;
}

UPPGameUserSettings* UPPGameUserSettings::GetPPGameUserSettings()
{
	return GEngine ? Cast<UPPGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UPPGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	SelectedGraphicsPreset = EPPGraphicsPreset::Medium;
	bHasExplicitGraphicsPreset = false;
	MouseSensitivity = DefaultMouseSensitivity;
	ScalabilityQuality = BuildQualityLevels(SelectedGraphicsPreset);
}

void UPPGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	if (bHasExplicitGraphicsPreset)
	{
		SelectedGraphicsPreset = SanitizePreset(static_cast<int32>(SelectedGraphicsPreset));
		ScalabilityQuality = BuildQualityLevels(SelectedGraphicsPreset);
	}
	else
	{
		// A missing project preset is a first-run install. Existing engine settings are
		// left untouched until the player explicitly chooses a project preset.
		SelectedGraphicsPreset = EPPGraphicsPreset::Medium;
	}

	MouseSensitivity = SanitizeMouseSensitivity(MouseSensitivity);
}

void UPPGameUserSettings::ApplyGraphicsPreset(EPPGraphicsPreset Preset)
{
	SelectedGraphicsPreset = SanitizePreset(static_cast<int32>(Preset));
	bHasExplicitGraphicsPreset = true;
	ScalabilityQuality = BuildQualityLevels(SelectedGraphicsPreset);
	ApplyNonResolutionSettings();
	ApplyResolutionSettings(false);
	SaveSettings();
	OnGraphicsPresetChanged.Broadcast(SelectedGraphicsPreset);
}

void UPPGameUserSettings::SetMouseSensitivity(float NewSensitivity)
{
	const float Sanitized = SanitizeMouseSensitivity(NewSensitivity);
	if (FMath::IsNearlyEqual(MouseSensitivity, Sanitized, 0.001f))
	{
		MouseSensitivity = Sanitized;
		return;
	}

	MouseSensitivity = Sanitized;
	SaveSettings();
	OnMouseSensitivityChanged.Broadcast(MouseSensitivity);
}

EPPGraphicsPreset UPPGameUserSettings::SanitizePreset(int32 RawValue)
{
	switch (RawValue)
	{
	case static_cast<int32>(EPPGraphicsPreset::Low):
		return EPPGraphicsPreset::Low;
	case static_cast<int32>(EPPGraphicsPreset::High):
		return EPPGraphicsPreset::High;
	case static_cast<int32>(EPPGraphicsPreset::Medium):
	default:
		return EPPGraphicsPreset::Medium;
	}
}

float UPPGameUserSettings::SanitizeMouseSensitivity(float RawValue)
{
	return FMath::Clamp(RawValue, MinMouseSensitivity, MaxMouseSensitivity);
}

Scalability::FQualityLevels UPPGameUserSettings::BuildQualityLevels(EPPGraphicsPreset Preset)
{
	Scalability::FQualityLevels Levels;
	const int32 GroupQuality = Preset == EPPGraphicsPreset::Low ? 1 : Preset == EPPGraphicsPreset::High ? 3 : 2;
	Levels.SetFromSingleQualityLevel(GroupQuality);
	Levels.ResolutionQuality = Preset == EPPGraphicsPreset::Low ? 50.0f : Preset == EPPGraphicsPreset::High ? 100.0f : 66.6667f;
	return Levels;
}
