// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowDownGameModeBase.h"

#include "Card.h"
#include "CardSystem.h"
#include "Collector.h"
#include "CollectorAISystem.h"
#include "BettingSystem.h"
#include "ShowDownTypes.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerPawn.h"

AShowDownGameModeBase::AShowDownGameModeBase()
{
	CardSystem = CreateDefaultSubobject<UCardSystem>(TEXT("CardSystem"));
	CollectorAISystem = CreateDefaultSubobject<UCollectorAISystem>(TEXT("CollectorAISystem"));
	BettingSystem = CreateDefaultSubobject<UBettingSystem>(TEXT("BettingSystem"));
}

void AShowDownGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	
	FindCollector();
	DealInitialHand();
}

void AShowDownGameModeBase::PlayerSelectedCard(ACard* SelectedCard)
{
	if (!SelectedCard){	return;}

	if (!CardSystem){
		UE_LOG(LogTemp, Warning, TEXT("CardSystem is missing on %s."), *GetName());
		return;
	}

	if (CollectorState.ForeheadCard){
		UE_LOG(LogTemp, Warning, TEXT("Collector already has a forehead card."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("GameMode received selected card: %s"), *SelectedCard->GetName());

	CardSystem->RemoveCardFromHand(PlayerState.HandCards, SelectedCard);
	//콜렉터의 이마로 카드 이동
	CollectorState.ForeheadCard = SelectedCard;

	if (!Collector || !Collector->c_HeadCard){
		UE_LOG(LogTemp, Warning, TEXT("Collector or Collector head card slot is missing."));
		return;
	}

	CardSystem->MoveCardToSlot(SelectedCard, Collector->c_HeadCard, true);
	
	CollectorGiveCardToPlayer();
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

void AShowDownGameModeBase::CollectorGiveCardToPlayer()
{
	if (!CardSystem){
		UE_LOG(LogTemp, Warning, TEXT("CardSystem is missing on %s."), *GetName());
		return;
	}

	if (!CollectorAISystem){
		UE_LOG(LogTemp, Warning, TEXT("CollectorAISystem is missing on %s."), *GetName());
		return;
	}

	if (PlayerState.ForeheadCard){
		UE_LOG(LogTemp, Warning, TEXT("Player already has a forehead card."));
		return;
	}

	APlayerPawn* PlayerPawn = Cast<APlayerPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (!PlayerPawn || !PlayerPawn->PlayerHeadCard){
		UE_LOG(LogTemp, Warning, TEXT("PlayerPawn or PlayerHeadCard is missing."));
		return;
	}

	if (CollectorState.HandCards.Num() <= 0){
		UE_LOG(LogTemp, Warning, TEXT("Collector has no hand cards."));
		return;
	}

	ACard* ChosenCard = CollectorAISystem->ChooseCardActorToGive(CollectorState.HandCards);
	if (!ChosenCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Collector AI failed to choose a card."));
		return;
	}

	CardSystem->RemoveCardFromHand(CollectorState.HandCards, ChosenCard);

	PlayerState.ForeheadCard = ChosenCard;

	// 플레이어는 자기 이마 카드를 보면 안 되므로 false
	CardSystem->MoveCardToSlot(ChosenCard, PlayerPawn->PlayerHeadCard, false);

	UE_LOG(LogTemp, Log, TEXT("Collector gave card to player: %s, Rank: %d"), *ChosenCard->GetName(), ChosenCard->Rank);

	StartBettingPhase();
}

void AShowDownGameModeBase::StartBettingPhase()
{
	if (!BettingSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("BettingSystem is missing on %s."), *GetName());
		return;
	}

	bBettingPhase = true;
	PlayerState.CurrentBet = 0;
	CollectorState.CurrentBet = 0;

	// 일단 최소 베팅 0으로 시작. 나중에 1로 바꿔도 됨.
	BettingSystem->ResetBetting(0);

	UE_LOG(LogTemp, Log, TEXT("Betting phase started. Q=Check, E=Raise, R=Fold"));
}

void AShowDownGameModeBase::PlayerCheck()
{
	if (!bBettingPhase || !BettingSystem || !CollectorAISystem)
	{
		return;
	}

	const int32 CurrentBet = BettingSystem->GetCurrentBet();
	if (CurrentBet > PlayerState.CurrentBet)
	{
		BettingSystem->Call(EShowDownSide::Player);
		PlayerState.CurrentBet = CurrentBet;
		bBettingPhase = false;

		UE_LOG(LogTemp, Log, TEXT("Player Call %d"), CurrentBet);
		UE_LOG(LogTemp, Log, TEXT("Betting finished. Next: Reveal cards."));
		return;
	}

	BettingSystem->Check(EShowDownSide::Player);
	PlayerState.CurrentBet = CurrentBet;

	UE_LOG(LogTemp, Log, TEXT("Player Check"));

	ResolveCollectorBetResponse();
}

void AShowDownGameModeBase::PlayerRaise()
{
	if (!bBettingPhase || !BettingSystem || !CollectorAISystem)
	{
		return;
	}

	const int32 CurrentBet = BettingSystem->GetCurrentBet();
	const int32 NewBet = FMath::Clamp(CurrentBet + 1, 1, 6);

	if (BettingSystem->RaiseTo(EShowDownSide::Player, NewBet))
	{
		PlayerState.CurrentBet = NewBet;

		UE_LOG(LogTemp, Log, TEXT("Player Raise to %d"), NewBet);

		ResolveCollectorBetResponse();
	}
}

void AShowDownGameModeBase::PlayerFold()
{
	if (!bBettingPhase || !BettingSystem)
	{
		return;
	}

	BettingSystem->Fold(EShowDownSide::Player);

	bBettingPhase = false;

	UE_LOG(LogTemp, Log, TEXT("Player Fold"));
	UE_LOG(LogTemp, Log, TEXT("Next: Roulette or round end."));
}

float AShowDownGameModeBase::EstimateCollectorWinChance() const
{
	if (!PlayerState.ForeheadCard || !CollectorState.ForeheadCard)
	{
		return 0.5f;
	}

	const int32 PlayerRank = PlayerState.ForeheadCard->Rank;
	const int32 CollectorRank = CollectorState.ForeheadCard->Rank;

	if (CollectorRank > PlayerRank)
	{
		return 0.8f;
	}

	if (CollectorRank < PlayerRank)
	{
		return 0.25f;
	}

	return 0.5f;
}

void AShowDownGameModeBase::ResolveCollectorBetResponse()
{
	if (!BettingSystem || !CollectorAISystem)
	{
		return;
	}

	const int32 CurrentBet = BettingSystem->GetCurrentBet();
	const int32 GivenCardRank = PlayerState.ForeheadCard ? PlayerState.ForeheadCard->Rank : 0;
	const EShowDownBetAction CollectorAction = CollectorAISystem->ChooseBetActionByGivenCard(GivenCardRank, CurrentBet);

	UE_LOG(LogTemp, Log, TEXT("Collector remembers given card rank: %d"), GivenCardRank);

	switch (CollectorAction)
	{
	case EShowDownBetAction::Check:
		BettingSystem->Check(EShowDownSide::Collector);
		CollectorState.CurrentBet = CurrentBet;
		bBettingPhase = false;
		UE_LOG(LogTemp, Log, TEXT("Collector Check"));
		UE_LOG(LogTemp, Log, TEXT("Betting finished. Next: Reveal cards."));
		break;

	case EShowDownBetAction::Call:
		BettingSystem->Call(EShowDownSide::Collector);
		CollectorState.CurrentBet = CurrentBet;
		bBettingPhase = false;
		UE_LOG(LogTemp, Log, TEXT("Collector Call %d"), CurrentBet);
		UE_LOG(LogTemp, Log, TEXT("Betting finished. Next: Reveal cards."));
		break;

	case EShowDownBetAction::Raise:
		{
			const int32 NewBet = FMath::Clamp(CurrentBet + 1, 1, 6);
			if (BettingSystem->RaiseTo(EShowDownSide::Collector, NewBet))
			{
				CollectorState.CurrentBet = NewBet;
				UE_LOG(LogTemp, Log, TEXT("Collector Raise to %d"), NewBet);
				UE_LOG(LogTemp, Log, TEXT("Player needs to respond."));
			}
			break;
		}

	case EShowDownBetAction::Fold:
		BettingSystem->Fold(EShowDownSide::Collector);
		bBettingPhase = false;
		UE_LOG(LogTemp, Log, TEXT("Collector Fold"));
		UE_LOG(LogTemp, Log, TEXT("Betting finished. Player wins this round."));
		break;

	default:
		break;
	}
}
