// Fill out your copyright notice in the Description page of Project Settings.


#include "CardSystem.h"


// Sets default values for this component's properties
UCardSystem::UCardSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCardSystem::ResetDeck()
{
	Deck.Reset();
	DiscardPile.Reset();
	BuildDeck();
}

void UCardSystem::ShuffleDeck()
{
	for (int32 Index = Deck.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Deck.Swap(Index, SwapIndex);
	}
}

bool UCardSystem::DealCards(int32 Count, TArray<int32>& OutCards)
{
	OutCards.Reset();

	if (Count <= 0 || Deck.Num() < Count)
	{
		return false;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		OutCards.Add(Deck[0]);
		Deck.RemoveAt(0);
	}

	return true;
}

void UCardSystem::DiscardCard(int32 Rank)
{
	DiscardPile.Add(Rank);
}

void UCardSystem::DiscardCards(const TArray<int32>& Ranks)
{
	for (const int32 Rank : Ranks)
	{
		DiscardCard(Rank);
	}
}

int32 UCardSystem::GetRemainingCardCount() const
{
	return Deck.Num();
}

void UCardSystem::BuildDeck()
{
	for (int32 Rank = 1; Rank <= 7; ++Rank)
	{
		Deck.Add(Rank);
		Deck.Add(Rank);
	}
}

