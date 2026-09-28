#include "Data/PPGameFlowSubsystem.h"

bool UPPGameFlowSubsystem::ConsumeReplayBypass()
{
	const bool bShouldBypass = bBypassOpeningMenuOnce;
	bBypassOpeningMenuOnce = false;
	return bShouldBypass;
}

void UPPGameFlowSubsystem::RequestPatrolMode(bool bTutorialMode)
{
	bHasRequestedPatrolMode = true;
	bRequestedTutorialMode = bTutorialMode;
}

bool UPPGameFlowSubsystem::ConsumeRequestedPatrolMode(bool& bOutTutorialMode)
{
	if (!bHasRequestedPatrolMode)
	{
		return false;
	}

	bOutTutorialMode = bRequestedTutorialMode;
	bHasRequestedPatrolMode = false;
	return true;
}
