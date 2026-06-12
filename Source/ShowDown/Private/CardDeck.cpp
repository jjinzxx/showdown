// Fill out your copyright notice in the Description page of Project Settings.


#include "CardDeck.h"


// Sets default values for this component's properties
UCardDeck::UCardDeck()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCardDeck::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCardDeck::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
void UCardDeck::ResetDeck()
{
	Deck.Reset();
	DiscardPile.Reset();

	BuildDeck();
}

void UCardDeck::BuildDeck()
{
	for (int32 Rank = 1; Rank <= 7; ++Rank)
	{
		Deck.Add(Rank);
		Deck.Add(Rank);
	}
}

void UCardDeck::ShuffleDeck()
{
	for (int32 Index = Deck.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Deck.Swap(Index, SwapIndex);
	}
}

bool UCardDeck::DealCards(int32 Count, TArray<int32>& OutCards)
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
