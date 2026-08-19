#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Scalability.h"
#include "PPGameUserSettings.generated.h"

UENUM(BlueprintType)
enum class EPPGraphicsPreset : uint8
{
	Low UMETA(DisplayName="Low"),
	Medium UMETA(DisplayName="Medium"),
	High UMETA(DisplayName="High")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPPGraphicsPresetChanged, EPPGraphicsPreset, Preset);

UCLASS(Config=GameUserSettings)
class KRUGER_CONCLUSION_API UPPGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UPPGameUserSettings();

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Graphics")
	EPPGraphicsPreset GetGraphicsPreset() const { return SelectedGraphicsPreset; }

	UFUNCTION(BlueprintCallable, Category="Poaching Patrol|Graphics")
	void ApplyGraphicsPreset(EPPGraphicsPreset Preset);

	UFUNCTION(BlueprintPure, Category="Poaching Patrol|Graphics", meta=(DisplayName="Get Poaching Patrol Game User Settings"))
	static UPPGameUserSettings* GetPPGameUserSettings();

	UPROPERTY(BlueprintAssignable, Category="Poaching Patrol|Graphics")
	FOnPPGraphicsPresetChanged OnGraphicsPresetChanged;

	virtual void SetToDefaults() override;
	virtual void LoadSettings(bool bForceReload = false) override;

	static EPPGraphicsPreset SanitizePreset(int32 RawValue);
	static Scalability::FQualityLevels BuildQualityLevels(EPPGraphicsPreset Preset);

private:
	UPROPERTY(Config)
	EPPGraphicsPreset SelectedGraphicsPreset = EPPGraphicsPreset::Medium;

	UPROPERTY(Config)
	bool bHasExplicitGraphicsPreset = false;
};
