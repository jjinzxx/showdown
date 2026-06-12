#include "SDPlayerState.h"

#include "Card.h"

void ASDPlayerState::AddHandCard(ACard* Card)
{
	if (Card)
	{
		HandCards.Add(Card);
	}
}

void ASDPlayerState::RemoveHandCard(ACard* Card)
{
	if (Card)
	{
		HandCards.Remove(Card);
	}
}

void ASDPlayerState::ClearHand()
{
	HandCards.Reset();
}
