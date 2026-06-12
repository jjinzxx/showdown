#include "CollectorAISystem.h"
#include "Card.h"

UCollectorAISystem::UCollectorAISystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UCollectorAISystem::ChooseCardToGive(const TArray<int32>& HandRanks) const
{
	if (HandRanks.Num() == 0){return 0;}

	TArray<int32> SortedRanks = HandRanks;
	SortedRanks.Sort();
	return SortedRanks[0];
}

ACard* UCollectorAISystem::ChooseCardActorToGive(const TArray<ACard*>& HandCards) const
{
	ACard* BestCard = nullptr;

	for (ACard* Card : HandCards)
	{
		if (!Card){continue;}

		if (!BestCard || Card->Rank < BestCard->Rank){
			BestCard = Card;
		}
	}

	return BestCard;
}



EShowDownBetAction UCollectorAISystem::ChooseBetAction(float EstimatedWinChance, int32 CurrentBet) const
{
	if (EstimatedWinChance > 0.7f && CurrentBet < 6)
	{
		return EShowDownBetAction::Raise;
	}

	if (EstimatedWinChance < 0.35f && CurrentBet > 2)
	{
		return EShowDownBetAction::Fold;
	}

	return CurrentBet == 0 ? EShowDownBetAction::Check : EShowDownBetAction::Call;
}
