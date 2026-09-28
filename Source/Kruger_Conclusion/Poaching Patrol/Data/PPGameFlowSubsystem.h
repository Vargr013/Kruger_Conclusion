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
	void RequestPatrolMode(bool bTutorialMode);
	void ClearRequestedPatrolMode() { bHasRequestedPatrolMode = false; }
	bool ConsumeRequestedPatrolMode(bool& bOutTutorialMode);

private:
	bool bBypassOpeningMenuOnce = false;
	bool bHasRequestedPatrolMode = false;
	bool bRequestedTutorialMode = false;
};
