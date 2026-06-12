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

EShowDownBetAction UCollectorAISystem::ChooseBetActionByGivenCard(int32 GivenCardRank, int32 CurrentBet) const
{
	if (GivenCardRank <= 0)
	{
		return CurrentBet == 0 ? EShowDownBetAction::Check : EShowDownBetAction::Call;
	}

	// 콜렉터는 자기가 플레이어에게 준 카드만 알고 판단합니다.
	if (GivenCardRank <= 3)
	{
		return CurrentBet < 6 ? EShowDownBetAction::Raise : EShowDownBetAction::Call;
	}

	return CurrentBet == 0 ? EShowDownBetAction::Check : EShowDownBetAction::Call;
}
