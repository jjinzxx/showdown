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
	const FSDCardHandLayoutSettings& HandLayoutSettings,
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
	for (int32 Index = 0; Index < CardsToDeal; ++Index)
	{
		const int32 Rank = Ranks[Index];
		const FTransform SpawnTransform = BuildHandCardTransform(HandRoot, HandLayoutSettings, Index, CardsToDeal);

		ACard* NewCard = World->SpawnActor<ACard>(CardClass, SpawnTransform);
		if (!NewCard)
		{
			continue;
		}

		NewCard->SetCard(Rank);
		NewCard->SetFaceUp(bFaceUp);
		NewCard->SetSelectable(bSelectable);
		NewCard->MoveToHandTransform(
			SpawnTransform,
			HandLayoutSettings.CameraFacingStrength,
			HandLayoutSettings.MaxCameraFacingAngle);
		OutCards.Add(NewCard);
	}

	return OutCards.Num() == CardsToDeal;
}

bool UCardSystem::LayoutHandCards(
	UObject* WorldContextObject,
	USceneComponent* HandRoot,
	const FSDCardHandLayoutSettings& HandLayoutSettings,
	const TArray<ACard*>& Cards)
{
	if (!WorldContextObject || !HandRoot)
	{
		return false;
	}

	const int32 CardCount = Cards.Num();
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		ACard* Card = Cards[Index];
		if (!IsValid(Card))
		{
			continue;
		}

		const FTransform TargetTransform = BuildHandCardTransform(HandRoot, HandLayoutSettings, Index, CardCount);
		Card->MoveToHandTransform(
			TargetTransform,
			HandLayoutSettings.CameraFacingStrength,
			HandLayoutSettings.MaxCameraFacingAngle);
	}

	return true;
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

FTransform UCardSystem::BuildHandCardTransform(
	USceneComponent* HandRoot,
	const FSDCardHandLayoutSettings& HandLayoutSettings,
	int32 CardIndex,
	int32 CardCount) const
{
	if (!HandRoot || CardCount <= 0)
	{
		return FTransform::Identity;
	}

	const FVector HandOrigin = HandRoot->GetComponentLocation();
	const FVector FanGripLocation = HandOrigin + HandRoot->GetForwardVector() * HandLayoutSettings.FanDistance;
	const FQuat HandRotation = HandRoot->GetComponentQuat();
	const FVector CardNormal = HandRoot->GetForwardVector();
	const FVector CardRight = HandRoot->GetRightVector();
	const FVector CardUp = HandRoot->GetUpVector();

	const float SafeFanWidth = FMath::Max(0.0f, HandLayoutSettings.FanWidth);
	const float SafeGripToCenterDistance = FMath::Max(1.0f, HandLayoutSettings.GripToCenterDistance);
	const float AutoFanAngle = FMath::Max(0.0f, HandLayoutSettings.AnglePerGap)
		* static_cast<float>(FMath::Max(0, CardCount - 1));
	float TotalFanAngle = HandLayoutSettings.HandLayoutStyle == ESDHandLayoutStyle::AutoFan
		? FMath::Min(AutoFanAngle, FMath::Max(0.0f, HandLayoutSettings.MaxFanAngle))
		: FMath::Max(0.0f, HandLayoutSettings.MaxFanAngle);

	if (CardCount > 1 && SafeFanWidth > 0.0f)
	{
		const float WidthAngleLimit = FMath::RadiansToDegrees(
			2.0f * FMath::Asin(FMath::Clamp(SafeFanWidth / (2.0f * SafeGripToCenterDistance), 0.0f, 1.0f)));
		TotalFanAngle = FMath::Min(TotalFanAngle, WidthAngleLimit);
	}

	const float NormalizedFromCenter = CardCount > 1
		? (static_cast<float>(CardIndex) / static_cast<float>(CardCount - 1)) * 2.0f - 1.0f
		: 0.0f;
	const float LayerFromCenter = static_cast<float>(CardIndex) - static_cast<float>(CardCount - 1) * 0.5f;
	const float AngleStep = CardCount > 1 ? TotalFanAngle / static_cast<float>(CardCount - 1) : 0.0f;
	const float FanAngle = -TotalFanAngle * 0.5f + AngleStep * CardIndex;

	const FQuat FanRotation(CardNormal, FMath::DegreesToRadians(FanAngle));
	const FVector FannedCardRight = FanRotation.RotateVector(CardRight);
	const FVector FannedCardUp = FanRotation.RotateVector(CardUp);
	const FQuat TiltRotation(FannedCardRight, FMath::DegreesToRadians(HandLayoutSettings.FaceTiltAngle));
	const FQuat CurlRotation(FannedCardUp, FMath::DegreesToRadians(-NormalizedFromCenter * HandLayoutSettings.EdgeCurlAngle));

	const FVector CardLocation =
		FanGripLocation
		+ FanRotation.RotateVector(CardUp * SafeGripToCenterDistance)
		+ CardNormal * (LayerFromCenter * HandLayoutSettings.LayerStep);
	const FQuat CardRotation = CurlRotation * TiltRotation * FanRotation * HandRotation;

	return FTransform(CardRotation, CardLocation);
}

