// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowDownGameModeBase.h"

#include "Card.h"
#include "CardDeck.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerPawn.h"

AShowDownGameModeBase::AShowDownGameModeBase()
{
	CardDeck = CreateDefaultSubobject<UCardDeck>(TEXT("CardDeck"));
}

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

	if (!CardDeck)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardDeck is missing on %s."), *GetName());
		return;
	}

	APlayerPawn* PlayerPawn = Cast<APlayerPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (!PlayerPawn || !PlayerPawn->PlayerHandCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerPawn or PlayerHandCard is missing."));
		return;
	}

	CardDeck->ResetDeck();
	CardDeck->ShuffleDeck();

	TArray<int32> PlayerRanks;
	if (!CardDeck->DealCards(HandCount, PlayerRanks))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to deal player hand."));
		return;
	}

	const int32 CardsToDeal = PlayerRanks.Num();
	const FVector HandCenter =
		PlayerPawn->PlayerHandCard->GetComponentLocation() +
		PlayerPawn->PlayerHandCard->GetForwardVector() * HandForwardOffset;
	const FRotator HandRotation = PlayerPawn->PlayerHandCard->GetComponentRotation();
	const FVector RightVector = PlayerPawn->PlayerHandCard->GetRightVector();
	const FVector ForwardVector = PlayerPawn->PlayerHandCard->GetForwardVector();
	const float StartOffset = -HandSpacing * static_cast<float>(CardsToDeal - 1) * 0.5f;
	const float AngleStep = CardsToDeal > 1 ? HandFanAngle / static_cast<float>(CardsToDeal - 1) : 0.0f;
	const float StartAngle = -HandFanAngle * 0.5f;

	for (int32 Index = 0; Index < CardsToDeal; ++Index)
	{
		const int32 Rank = PlayerRanks[Index];
		const float NormalizedFromCenter = CardsToDeal > 1
			? (static_cast<float>(Index) / static_cast<float>(CardsToDeal - 1)) * 2.0f - 1.0f
			: 0.0f;
		const float FanDepthOffset = FMath::Abs(NormalizedFromCenter) * HandFanDepth;
		const FVector SpawnLocation =
			HandCenter +
			RightVector * (StartOffset + HandSpacing * Index) -
			ForwardVector * FanDepthOffset;
		const FRotator SpawnRotation = HandRotation + FRotator(0.0f, StartAngle + AngleStep * Index, 0.0f);

		ACard* NewCard = GetWorld()->SpawnActor<ACard>(CardClass, SpawnLocation, SpawnRotation);
		if (!NewCard)
		{
			continue;
		}

		NewCard->SetCard(Rank);
		NewCard->SetFaceUp(true);
	}
}