// Fill out your copyright notice in the Description page of Project Settings.


#include "CardSystem.h"

#include "Card.h"

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

bool UCardSystem::SpawnHandCards(
	UObject* WorldContextObject,
	TSubclassOf<ACard> CardClass,
	USceneComponent* HandRoot,
	const TArray<int32>& Ranks,
	float HandSpacing,
	float HandForwardOffset,
	float HandFanAngle,
	float HandFanDepth,
	bool bFaceUp,
	bool bSelectable,
	TArray<ACard*>& OutCards)
{
	OutCards.Reset();

	if (!WorldContextObject || !CardClass || !HandRoot)
	{
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	const int32 CardsToDeal = Ranks.Num();
	const FVector HandCenter =
		HandRoot->GetComponentLocation() +
		HandRoot->GetForwardVector() * HandForwardOffset;
	const FRotator HandRotation = HandRoot->GetComponentRotation();
	const FVector RightVector = HandRoot->GetRightVector();
	const FVector ForwardVector = HandRoot->GetForwardVector();
	const float StartOffset = -HandSpacing * static_cast<float>(CardsToDeal - 1) * 0.5f;
	const float AngleStep = CardsToDeal > 1 ? HandFanAngle / static_cast<float>(CardsToDeal - 1) : 0.0f;
	const float StartAngle = -HandFanAngle * 0.5f;

	for (int32 Index = 0; Index < CardsToDeal; ++Index)
	{
		const int32 Rank = Ranks[Index];
		const float NormalizedFromCenter = CardsToDeal > 1
			? (static_cast<float>(Index) / static_cast<float>(CardsToDeal - 1)) * 2.0f - 1.0f
			: 0.0f;
		const float FanDepthOffset = FMath::Abs(NormalizedFromCenter) * HandFanDepth;
		const FVector SpawnLocation =
			HandCenter +
			RightVector * (StartOffset + HandSpacing * Index) -
			ForwardVector * FanDepthOffset;
		const FRotator SpawnRotation = HandRotation + FRotator(0.0f, StartAngle + AngleStep * Index, 0.0f);

		ACard* NewCard = World->SpawnActor<ACard>(CardClass, SpawnLocation, SpawnRotation);
		if (!NewCard)
		{
			continue;
		}

		NewCard->SetCard(Rank);
		NewCard->SetFaceUp(bFaceUp);
		NewCard->SetSelectable(bSelectable);
		OutCards.Add(NewCard);
	}

	return OutCards.Num() == CardsToDeal;
}

bool UCardSystem::RemoveCardFromHand(TArray<ACard*>& HandCards, ACard* Card)
{
	if (!Card)
	{
		return false;
	}

	return HandCards.Remove(Card) > 0;
}

bool UCardSystem::MoveCardToSlot(ACard* Card, USceneComponent* Slot, bool bFaceUp)
{
	if (!Card || !Slot)
	{
		return false;
	}

	Card->MoveToSlot(Slot, bFaceUp);
	return true;
}

void UCardSystem::BuildDeck()
{
	for (int32 Rank = 1; Rank <= 7; ++Rank)
	{
		Deck.Add(Rank);
		Deck.Add(Rank);
	}
}

