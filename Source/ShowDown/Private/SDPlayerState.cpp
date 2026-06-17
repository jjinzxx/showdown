#include "SDPlayerState.h"

#include "Card.h"
#include "Net/UnrealNetwork.h"

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

void ASDPlayerState::SetShowDownSlot(EShowDownPlayerSlot NewSlot)
{
	if (HasAuthority())
	{
		ShowDownSlot = NewSlot;
	}
}

void ASDPlayerState::SetReady(bool bNewReady)
{
	if (HasAuthority())
	{
		bReady = bNewReady;
	}
}

void ASDPlayerState::SetHostPlayer(bool bNewHostPlayer)
{
	if (HasAuthority())
	{
		bHostPlayer = bNewHostPlayer;
	}
}

void ASDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASDPlayerState, HandCards);
	DOREPLIFETIME(ASDPlayerState, ForeheadCard);
	DOREPLIFETIME(ASDPlayerState, Lives);
	DOREPLIFETIME(ASDPlayerState, CurrentBet);
	DOREPLIFETIME(ASDPlayerState, ShowDownSlot);
	DOREPLIFETIME(ASDPlayerState, bReady);
	DOREPLIFETIME(ASDPlayerState, bHostPlayer);
}
