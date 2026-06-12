// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowDownGameModeBase.h"

#include "Card.h"
#include "CardSystem.h"
#include "Collector.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerPawn.h"

AShowDownGameModeBase::AShowDownGameModeBase()
{
	CardSystem = CreateDefaultSubobject<UCardSystem>(TEXT("CardSystem"));
}

void AShowDownGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	
	FindCollector();
	DealInitialHand();
}

void AShowDownGameModeBase::PlayerSelectedCard(ACard* SelectedCard)
{
	if (!SelectedCard)
	{
		return;
	}

	if (!CardSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardSystem is missing on %s."), *GetName());
		return;
	}

	if (CollectorState.ForeheadCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Collector already has a forehead card."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("GameMode received selected card: %s"), *SelectedCard->GetName());

	CardSystem->RemoveCardFromHand(PlayerState.HandCards, SelectedCard);
	//콜렉터의 이마로 카드 이동
	CollectorState.ForeheadCard = SelectedCard;

	if (!Collector || !Collector->c_HeadCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Collector or Collector head card slot is missing."));
		return;
	}

	CardSystem->MoveCardToSlot(SelectedCard, Collector->c_HeadCard, true);
}

void AShowDownGameModeBase::DealInitialHand()
{
	if (!CardClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardClass is not assigned on %s."), *GetName());
		return;
	}

	if (!CardSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardSystem is missing on %s."), *GetName());
		return;
	}

	APlayerPawn* PlayerPawn = Cast<APlayerPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (!PlayerPawn || !PlayerPawn->PlayerHandCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerPawn or PlayerHandCard is missing."));
		return;
	}

	if (!Collector || !Collector->c_HandCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Collector or Collector hand card slot is missing."));
		return;
	}

	CardSystem->ResetDeck();
	CardSystem->ShuffleDeck();

	TArray<int32> PlayerRanks;
	if (!CardSystem->DealCards(HandCount, PlayerRanks))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to deal player hand."));
		return;
	}

	TArray<int32> CollectorRanks;
	if (!CardSystem->DealCards(HandCount, CollectorRanks))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to deal collector hand."));
		return;
	}

	CardSystem->SpawnHandCards(
		this,
		CardClass,
		PlayerPawn->PlayerHandCard,
		PlayerRanks,
		HandSpacing,
		HandForwardOffset,
		HandFanAngle,
		HandFanDepth,
		true,
		true,
		PlayerState.HandCards);

	// 확인용으로 true. 나중에는 false로 바꾸면 콜렉터 손패가 뒷면이 됩니다.
	CardSystem->SpawnHandCards(
		this,
		CardClass,
		Collector->c_HandCard,
		CollectorRanks,
		HandSpacing,
		HandForwardOffset,
		HandFanAngle,
		HandFanDepth,
		true,
		false,
		CollectorState.HandCards);

	UE_LOG(LogTemp, Log, TEXT("Player hand count: %d"), PlayerState.HandCards.Num());
	UE_LOG(LogTemp, Log, TEXT("Collector hand count: %d"), CollectorState.HandCards.Num());
}

void AShowDownGameModeBase::FindCollector()
{
	Collector = Cast<ACollector>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ACollector::StaticClass())
	);

	if (!Collector)
	{
		UE_LOG(LogTemp, Warning, TEXT("Collector is missing in the level."));
	}
}
