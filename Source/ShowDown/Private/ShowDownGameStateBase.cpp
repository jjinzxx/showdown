#include "ShowDownGameStateBase.h"

void AShowDownGameStateBase::SetPhase(EShowDownPhase NewPhase)
{
	if (CurrentPhase == NewPhase)
	{
		return;
	}

	CurrentPhase = NewPhase;
	OnPhaseChanged.Broadcast(CurrentPhase);
}
