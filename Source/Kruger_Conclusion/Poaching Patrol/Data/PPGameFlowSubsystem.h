#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PPGameFlowSubsystem.generated.h"

UCLASS()
class KRUGER_CONCLUSION_API UPPGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void RequestReplayBypass() { bBypassOpeningMenuOnce = true; }
	void ClearReplayBypass() { bBypassOpeningMenuOnce = false; }
	bool ConsumeReplayBypass();

private:
	bool bBypassOpeningMenuOnce = false;
};
