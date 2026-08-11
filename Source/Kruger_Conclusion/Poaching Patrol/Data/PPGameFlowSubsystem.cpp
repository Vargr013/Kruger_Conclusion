#include "Data/PPGameFlowSubsystem.h"

bool UPPGameFlowSubsystem::ConsumeReplayBypass()
{
	const bool bShouldBypass = bBypassOpeningMenuOnce;
	bBypassOpeningMenuOnce = false;
	return bShouldBypass;
}
