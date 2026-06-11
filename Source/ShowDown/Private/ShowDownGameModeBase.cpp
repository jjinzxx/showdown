// Fill out your copyright notice in the Description page of Project Settings.


#include "ShowDownGameModeBase.h"

#include "Card.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerPawn.h"

void AShowDownGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	DealPlayerInitialHand();
}

void AShowDownGameModeBase::PlayerSelectedCard(ACard* SelectedCard)
{
	if (!SelectedCard)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("GameMode received selected card: %s"), *SelectedCard->GetName());
}

void AShowDownGameModeBase::DealPlayerInitialHand()
{
	if (!CardClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardClass is not assigned on %s."), *GetName());
		return;
	}

	APlayerPawn* PlayerPawn = Cast<APlayerPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (!PlayerPawn || !PlayerPawn->PlayerHandCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerPawn or PlayerHandCard is missing."));
		return;
	}

	TArray<int32> Deck;
	BuildDeck(Deck);
	ShuffleDeck(Deck);

	const int32 CardsToDeal = FMath::Min(PlayerHandCount, Deck.Num());
	const FVector HandCenter =
		PlayerPawn->PlayerHandCard->GetComponentLocation() +
		PlayerPawn->PlayerHandCard->GetForwardVector() * PlayerHandForwardOffset;
	const FRotator HandRotation = PlayerPawn->PlayerHandCard->GetComponentRotation();
	const FVector RightVector = PlayerPawn->PlayerHandCard->GetRightVector();
	const FVector ForwardVector = PlayerPawn->PlayerHandCard->GetForwardVector();
	const float StartOffset = -PlayerHandSpacing * static_cast<float>(CardsToDeal - 1) * 0.5f;
	const float AngleStep = CardsToDeal > 1 ? PlayerHandFanAngle / static_cast<float>(CardsToDeal - 1) : 0.0f;
	const float StartAngle = -PlayerHandFanAngle * 0.5f;

	for (int32 Index = 0; Index < CardsToDeal; ++Index)
	{
		const int32 Rank = Deck[Index];
		const float NormalizedFromCenter = CardsToDeal > 1
			? (static_cast<float>(Index) / static_cast<float>(CardsToDeal - 1)) * 2.0f - 1.0f
			: 0.0f;
		const float FanDepthOffset = FMath::Abs(NormalizedFromCenter) * PlayerHandFanDepth;
		const FVector SpawnLocation =
			HandCenter +
			RightVector * (StartOffset + PlayerHandSpacing * Index) -
			ForwardVector * FanDepthOffset;
		const FRotator SpawnRotation = HandRotation + FRotator(0.0f, StartAngle + AngleStep * Index, 0.0f);

		ACard* NewCard = GetWorld()->SpawnActor<ACard>(CardClass, SpawnLocation, SpawnRotation);
		if (!NewCard)
		{
			continue;
		}

		NewCard->SetCard(Index % 2 == 0 ? ECardSuit::Spade : ECardSuit::Heart, Rank);
		NewCard->SetFaceUp(true);
	}
}

void AShowDownGameModeBase::BuildDeck(TArray<int32>& OutDeck) const
{
	OutDeck.Reset();

	for (int32 Rank = 1; Rank <= 7; ++Rank)
	{
		OutDeck.Add(Rank);
		OutDeck.Add(Rank);
	}
}

void AShowDownGameModeBase::ShuffleDeck(TArray<int32>& Deck) const
{
	for (int32 Index = Deck.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Deck.Swap(Index, SwapIndex);
	}
}
