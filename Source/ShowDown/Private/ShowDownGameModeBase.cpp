// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowDownGameModeBase.h"

#include "Card.h"
#include "CardSystem.h"
#include "Collector.h"
#include "CollectorAISystem.h"
#include "BettingSystem.h"
#include "RoundResolver.h"
#include "RouletteSystem.h"
#include "ShowDownTypes.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerPawn.h"

AShowDownGameModeBase::AShowDownGameModeBase()
{
	CardSystem = CreateDefaultSubobject<UCardSystem>(TEXT("CardSystem"));
	CollectorAISystem = CreateDefaultSubobject<UCollectorAISystem>(TEXT("CollectorAISystem"));
	BettingSystem = CreateDefaultSubobject<UBettingSystem>(TEXT("BettingSystem"));
	RoundResolver = CreateDefaultSubobject<URoundResolver>(TEXT("RoundResolver"));
	RouletteSystem = CreateDefaultSubobject<URouletteSystem>(TEXT("RouletteSystem"));
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
	SetPlayerHandSelectable(false);

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
		FinishBettingAndResolveRound();
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
	ResolveFold(EShowDownSide::Player);
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
		FinishBettingAndResolveRound();
		break;

	case EShowDownBetAction::Call:
		BettingSystem->Call(EShowDownSide::Collector);
		CollectorState.CurrentBet = CurrentBet;
		bBettingPhase = false;
		UE_LOG(LogTemp, Log, TEXT("Collector Call %d"), CurrentBet);
		FinishBettingAndResolveRound();
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
		ResolveFold(EShowDownSide::Collector);
		break;

	default:
		break;
	}
}

void AShowDownGameModeBase::FinishBettingAndResolveRound()
{
	if (!RoundResolver || !PlayerState.ForeheadCard || !CollectorState.ForeheadCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot resolve round. RoundResolver or forehead cards are missing."));
		return;
	}

	PlayerState.ForeheadCard->SetFaceUp(true);
	CollectorState.ForeheadCard->SetFaceUp(true);

	const int32 PlayerCardRank = PlayerState.ForeheadCard->Rank;
	const int32 CollectorCardRank = CollectorState.ForeheadCard->Rank;
	const EShowDownRoundResult Result = RoundResolver->ResolveRevealedCards(PlayerCardRank, CollectorCardRank);

	UE_LOG(LogTemp, Log, TEXT("Reveal cards. Player: %d, Collector: %d"), PlayerCardRank, CollectorCardRank);
	UE_LOG(LogTemp, Log, TEXT("Roulette will start in 5 seconds."));

	GetWorldTimerManager().SetTimer(
		RevealDelayHandle,
		FTimerDelegate::CreateUObject(this, &AShowDownGameModeBase::ContinueRoundAfterReveal, Result),
		5.0f,
		false);
}

void AShowDownGameModeBase::ContinueRoundAfterReveal(EShowDownRoundResult Result)
{
	switch (Result)
	{
	case EShowDownRoundResult::PlayerWin:
		UE_LOG(LogTemp, Log, TEXT("Round result: Player wins. Collector roulette with %d bullet(s)."), CollectorState.CurrentBet);
		ApplyRouletteResult(EShowDownSide::Collector, CollectorState.CurrentBet);
		EndRound();
		break;

	case EShowDownRoundResult::CollectorWin:
		UE_LOG(LogTemp, Log, TEXT("Round result: Collector wins. Player roulette with %d bullet(s)."), PlayerState.CurrentBet);
		ApplyRouletteResult(EShowDownSide::Player, PlayerState.CurrentBet);
		EndRound();
		break;

	case EShowDownRoundResult::Draw:
		UE_LOG(LogTemp, Log, TEXT("Round result: Draw. Both sides roulette."));
		ApplyRouletteResult(EShowDownSide::Collector, CollectorState.CurrentBet);
		ApplyRouletteResult(EShowDownSide::Player, PlayerState.CurrentBet);
		EndRound();
		break;

	default:
		break;
	}
}

void AShowDownGameModeBase::ResolveFold(EShowDownSide FoldedSide)
{
	if (!RoundResolver)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot resolve fold. RoundResolver is missing."));
		return;
	}

	FShowDownParticipantState& FoldedState = FoldedSide == EShowDownSide::Player ? PlayerState : CollectorState;
	const int32 FoldedCardRank = FoldedState.ForeheadCard ? FoldedState.ForeheadCard->Rank : 0;
	const int32 LoadCount = RoundResolver->GetFoldLoadCount(FoldedCardRank, FoldedState.CurrentBet);

	if (PlayerState.ForeheadCard)
	{
		PlayerState.ForeheadCard->SetFaceUp(true);
	}

	if (CollectorState.ForeheadCard)
	{
		CollectorState.ForeheadCard->SetFaceUp(true);
	}

	UE_LOG(LogTemp, Log, TEXT("%s folded. Forehead card: %d, roulette load: %d"),
		FoldedSide == EShowDownSide::Player ? TEXT("Player") : TEXT("Collector"),
		FoldedCardRank,
		LoadCount);
	UE_LOG(LogTemp, Log, TEXT("Roulette will start in 5 seconds."));

	GetWorldTimerManager().SetTimer(
		RevealDelayHandle,
		FTimerDelegate::CreateUObject(this, &AShowDownGameModeBase::ContinueFoldAfterReveal, FoldedSide, LoadCount),
		5.0f,
		false);
}

void AShowDownGameModeBase::ContinueFoldAfterReveal(EShowDownSide FoldedSide, int32 LoadCount)
{
	ApplyRouletteResult(FoldedSide, LoadCount);
	EndRound();
}

void AShowDownGameModeBase::ApplyRouletteResult(EShowDownSide TargetSide, int32 BulletCount)
{
	if (!RouletteSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot roll roulette. RouletteSystem is missing."));
		return;
	}

	const int32 ClampedBulletCount = FMath::Clamp(BulletCount, 0, 6);
	const float HitChance = RouletteSystem->GetHitChance(ClampedBulletCount);
	const bool bHit = RouletteSystem->RollRoulette(ClampedBulletCount);

	UE_LOG(LogTemp, Log, TEXT("%s roulette: %d/6, %.0f%% chance, result: %s"),
		TargetSide == EShowDownSide::Player ? TEXT("Player") : TEXT("Collector"),
		ClampedBulletCount,
		HitChance * 100.0f,
		bHit ? TEXT("Hit") : TEXT("Miss"));

	if (!bHit)
	{
		return;
	}

	FShowDownParticipantState& TargetState = TargetSide == EShowDownSide::Player ? PlayerState : CollectorState;
	TargetState.Lives = FMath::Max(0, TargetState.Lives - 1);

	UE_LOG(LogTemp, Log, TEXT("%s lives: %d"),
		TargetSide == EShowDownSide::Player ? TEXT("Player") : TEXT("Collector"),
		TargetState.Lives);
}

void AShowDownGameModeBase::EndRound()
{
	bBettingPhase = false;

	ClearForeheadCards();

	if (PlayerState.Lives <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Game Over. Player has no lives left."));
		return;
	}

	if (CollectorState.Lives <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Player wins. Collector has no lives left."));
		return;
	}

	const bool bNeedRedeal = PlayerState.HandCards.Num() <= 0 || CollectorState.HandCards.Num() <= 0;
	if (bNeedRedeal)
	{
		UE_LOG(LogTemp, Log, TEXT("Hands are empty. Shuffling and dealing new 5-card hands."));
		DealInitialHand();
		return;
	}

	SetPlayerHandSelectable(true);
	UE_LOG(LogTemp, Log, TEXT("Next round ready. Player hand: %d, Collector hand: %d"),
		PlayerState.HandCards.Num(),
		CollectorState.HandCards.Num());
}

void AShowDownGameModeBase::ClearForeheadCards()
{
	TArray<int32> DiscardRanks;

	if (PlayerState.ForeheadCard)
	{
		DiscardRanks.Add(PlayerState.ForeheadCard->Rank);
		PlayerState.ForeheadCard->Destroy();
		PlayerState.ForeheadCard = nullptr;
	}

	if (CollectorState.ForeheadCard)
	{
		DiscardRanks.Add(CollectorState.ForeheadCard->Rank);
		CollectorState.ForeheadCard->Destroy();
		CollectorState.ForeheadCard = nullptr;
	}

	if (CardSystem && DiscardRanks.Num() > 0)
	{
		CardSystem->DiscardCards(DiscardRanks);
	}
}

void AShowDownGameModeBase::SetPlayerHandSelectable(bool bSelectable)
{
	for (ACard* Card : PlayerState.HandCards)
	{
		if (Card)
		{
			Card->SetSelectable(bSelectable);
		}
	}
}
