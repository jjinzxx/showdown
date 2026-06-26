// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowDownGameModeBase.h"

#include "Card.h"
#include "CardSystem.h"
#include "Collector.h"
#include "CollectorAISystem.h"
#include "BettingSystem.h"
#include "RoundResolver.h"
#include "RouletteSystem.h"
#include "SDCardPlacementAnchor.h"
#include "SDLLMSubsystem.h"
#include "ShowDownTypes.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerPawn.h"
#include "SDPlayerSeat.h"
#include "SDPlayerState.h"
#include "ShowDownGameStateBase.h"
#include "ShowDownHubFlowManager.h"
#include "ShowDownPlayerController.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerState.h"

AShowDownGameModeBase::AShowDownGameModeBase()
{
	// GameState를 AShowDownGameStateBase로 고정합니다.
	// 이게 없으면 기본 AGameStateBase가 생성되어 GetGameState<AShowDownGameStateBase>()가
	// 항상 null이 되고, OnGameOver를 포함한 모든 GameState 이벤트가 broadcast되지 않습니다.
	GameStateClass = AShowDownGameStateBase::StaticClass();
	PlayerStateClass = ASDPlayerState::StaticClass();
	PlayerControllerClass = AShowDownPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	CardClass = ACard::StaticClass();

	CardSystem = CreateDefaultSubobject<UCardSystem>(TEXT("CardSystem"));
	CollectorAISystem = CreateDefaultSubobject<UCollectorAISystem>(TEXT("CollectorAISystem"));
	BettingSystem = CreateDefaultSubobject<UBettingSystem>(TEXT("BettingSystem"));
	RoundResolver = CreateDefaultSubobject<URoundResolver>(TEXT("RoundResolver"));
	RouletteSystem = CreateDefaultSubobject<URouletteSystem>(TEXT("RouletteSystem"));

	FShowDownStageRule Stage1;
	Stage1.StartingLives = 3;
	Stage1.MinimumBet = 1;
	Stage1.CollectorBluffRate = 0.15f;
	Stage1.CollectorAggression = 0.5f;
	Stage1.bSevenFoldLoadsSix = true;

	FShowDownStageRule Stage2;
	Stage2.StartingLives = 3;
	Stage2.MinimumBet = 1;
	Stage2.CollectorBluffRate = 0.25f;
	Stage2.CollectorAggression = 0.55f;
	Stage2.bSevenFoldLoadsSix = true;

	FShowDownStageRule Stage3;
	Stage3.StartingLives = 3;
	Stage3.MinimumBet = 2;
	Stage3.CollectorBluffRate = 0.3f;
	Stage3.CollectorAggression = 0.75f;
	Stage3.bSevenFoldLoadsSix = true;

	StageRules = { Stage1, Stage2, Stage3 };
}

void AShowDownGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// 레벨에 HubFlowManager가 있으면 싱글플레이 버튼을 누를 때까지 시작을 미룹니다.
	// 허브가 없는 테스트 레벨(ShowDown_Test 등)에서는 기존처럼 곧장 시작합니다.
	const bool bHubControlsStart =
		UGameplayStatics::GetActorOfClass(GetWorld(), AShowDownHubFlowManager::StaticClass()) != nullptr;

	const bool bNetworkedMatch = GetNetMode() != NM_Standalone;
	if (bNetworkedMatch)
	{
		StartMultiplayerGame();
		return;
	}

	if (bAutoStartOnBeginPlay && !bHubControlsStart)
	{
		StartSinglePlayer();
	}
}

void AShowDownGameModeBase::StartSinglePlayer()
{
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->SetMatchMode(EShowDownMatchMode::SinglePlayer);
	}

	FindCollector();
	StartStage(0);
}

void AShowDownGameModeBase::StartMultiplayerGame()
{
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->SetMatchMode(EShowDownMatchMode::Multiplayer);
	}

	FindCollector();
	RefreshNetworkPlayerSlots();

	// Gameplay rules for real PvP can be layered on this entry point.
	// For now this keeps multiplayer from falling back into the single-player AI flow.
	UE_LOG(LogTemp, Log, TEXT("Multiplayer game structure initialized."));
}

void AShowDownGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (GetNetMode() != NM_Standalone)
	{
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->SetMatchMode(EShowDownMatchMode::Multiplayer);
		}
	}

	if (ASDPlayerState* ShowDownPlayerState = NewPlayer ? NewPlayer->GetPlayerState<ASDPlayerState>() : nullptr)
	{
		if (ShowDownPlayerState->ShowDownSlot == EShowDownPlayerSlot::None)
		{
			ShowDownPlayerState->SetShowDownSlot(FindNextOpenPlayerSlot());
		}
		ShowDownPlayerState->SetHostPlayer(GameState && GameState->PlayerArray.Num() <= 1);
	}

	RefreshNetworkPlayerSlots();
}

void AShowDownGameModeBase::Logout(AController* Exiting)
{
	if (ASDPlayerState* ShowDownPlayerState = Exiting ? Exiting->GetPlayerState<ASDPlayerState>() : nullptr)
	{
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->ClearPlayerSlot(ShowDownPlayerState->ShowDownSlot);
		}
	}

	Super::Logout(Exiting);
	RefreshNetworkPlayerSlots();
}

void AShowDownGameModeBase::ResetForHubReturn()
{
	GetWorldTimerManager().ClearTimer(CardPlacementDelayHandle);
	GetWorldTimerManager().ClearTimer(RevealDelayHandle);
	GetWorldTimerManager().ClearTimer(CollectorActionPresentationTimerHandle);

	bBettingPhase = false;
	bHasPendingRoundReveal = false;
	bHasPendingFoldReveal = false;
	bCollectorActionPresentationInProgress = false;
	CardPlacementDelayContinuation = TFunction<void()>();
	CollectorActionPresentationContinuation = TFunction<void()>();
	QueuedCollectorActionPresentationContinuations.Reset();

	ClearForeheadCards();
	ClearHandCards();

	UE_LOG(LogTemp, Log, TEXT("Board reset for hub return."));
}

void AShowDownGameModeBase::PlayerSelectedCard(ACard* SelectedCard)
{
	if (bCollectorActionPresentationInProgress)
	{
		return;
	}

	if (!IsValid(SelectedCard))
	{
		return;
	}

	if (!CardSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardSystem is missing on %s."), *GetName());
		return;
	}

	if (!SelectedCard->IsCardSelectable() || !PlayerState.HandCards.Contains(SelectedCard))
	{
		UE_LOG(LogTemp, Warning, TEXT("Rejected selected card %s because it is not in the player's selectable hand."),
			*SelectedCard->GetName());
		return;
	}

	if (CollectorState.ForeheadCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Collector already has a forehead card."));
		return;
	}

	USceneComponent* CollectorHeadSlot = GetHeadSlotForSide(EShowDownSide::Collector);
	if (!CollectorHeadSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("Opponent forehead slot is missing. Add an SDCardPlacementAnchor with PlacementRole=OpponentForehead, or keep an old fallback slot."));
		return;
	}

	CurrentRoundPlayerGaveRank = SelectedCard->Rank;
	RecordCurrentRoundAction(FString::Printf(TEXT("Player gave Collector forehead card rank %d."), CurrentRoundPlayerGaveRank));
	UE_LOG(LogTemp, Log, TEXT("GameMode received selected card: %s"), *SelectedCard->GetName());

	CardSystem->RemoveCardFromHand(PlayerState.HandCards, SelectedCard);
	ReflowHandCards(EShowDownSide::Player);
	//콜렉터의 이마로 카드 이동
	CollectorState.ForeheadCard = SelectedCard;

	CardSystem->MoveCardToSlot(SelectedCard, CollectorHeadSlot, true);

	WaitForCardPlacementThen(SelectedCard, [this]()
	{
		PlayCollectorActionPresentationThen([this]()
		{
			if (PlayerState.ForeheadCard)
			{
				StartBettingPhase();
			}
			else
			{
				CollectorGiveCardToPlayer();
			}
		});
	});
}

void AShowDownGameModeBase::DealInitialHand()
{
	if (!CardClass)
	{
		CardClass = ACard::StaticClass();
	}

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

	USceneComponent* PlayerHandSlot = GetHandSlotForSide(EShowDownSide::Player);
	if (!PlayerHandSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player hand slot is missing. Add an SDCardPlacementAnchor with PlacementRole=PlayerHand, or keep an old fallback slot."));
		return;
	}

	USceneComponent* CollectorHandSlot = GetHandSlotForSide(EShowDownSide::Collector);
	if (!CollectorHandSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("Opponent hand slot is missing. Add an SDCardPlacementAnchor with PlacementRole=OpponentHand, or keep an old fallback slot."));
		return;
	}

	const FSDCardHandLayoutSettings PlayerHandLayout = ResolveHandLayoutSettings(EShowDownSide::Player);
	const FSDCardHandLayoutSettings CollectorHandLayout = ResolveHandLayoutSettings(EShowDownSide::Collector);

	ClearHandCards();
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
		PlayerHandSlot,
		PlayerRanks,
		PlayerHandLayout,
		true,
		true,
		PlayerState.HandCards);

	ApplyCardMotionForSide(EShowDownSide::Player, PlayerState.HandCards);

	// 확인용으로 true. 나중에는 false로 바꾸면 콜렉터 손패가 뒷면이 됩니다.
	CardSystem->SpawnHandCards(
		this,
		CardClass,
		CollectorHandSlot,
		CollectorRanks,
		CollectorHandLayout,
		true,
		false,
		CollectorState.HandCards);
	ApplyCardMotionForSide(EShowDownSide::Collector, CollectorState.HandCards);

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
		if (!GetHandAnchorForSide(EShowDownSide::Collector)
			&& !GetForeheadAnchorForSide(EShowDownSide::Collector)
			&& !GetSeatForSide(EShowDownSide::Collector))
		{
			UE_LOG(LogTemp, Warning, TEXT("No opponent placement found. Add OpponentHand/OpponentForehead SDCardPlacementAnchor actors, or keep an old fallback slot."));
		}
	}
}

void AShowDownGameModeBase::PlayCollectorActionPresentation()
{
	PlayCollectorActionPresentationThen(TFunction<void()>());
}

void AShowDownGameModeBase::WaitForCardPlacementThen(ACard* Card, TFunction<void()>&& Continuation)
{
	CardPlacementDelayContinuation = MoveTemp(Continuation);

	const float PlacementDelay = IsValid(Card) ? Card->GetSlotAttachMotionTotalSeconds() : 0.0f;
	if (PlacementDelay <= KINDA_SMALL_NUMBER)
	{
		FinishCardPlacementWait();
		return;
	}

	GetWorldTimerManager().ClearTimer(CardPlacementDelayHandle);
	GetWorldTimerManager().SetTimer(
		CardPlacementDelayHandle,
		this,
		&AShowDownGameModeBase::FinishCardPlacementWait,
		PlacementDelay,
		false);
}

void AShowDownGameModeBase::FinishCardPlacementWait()
{
	GetWorldTimerManager().ClearTimer(CardPlacementDelayHandle);

	TFunction<void()> Continuation = MoveTemp(CardPlacementDelayContinuation);
	CardPlacementDelayContinuation = TFunction<void()>();
	if (Continuation)
	{
		Continuation();
	}
}

void AShowDownGameModeBase::PlayCollectorActionPresentationThen(TFunction<void()>&& Continuation)
{
	if (bCollectorActionPresentationInProgress)
	{
		QueuedCollectorActionPresentationContinuations.Add(MoveTemp(Continuation));
		return;
	}

	if (!Collector)
	{
		FindCollector();
	}

	if (!Collector || !Collector->bEnableActionSpin)
	{
		if (Continuation)
		{
			Continuation();
		}
		return;
	}

	bCollectorActionPresentationInProgress = true;
	CollectorActionPresentationContinuation = MoveTemp(Continuation);

	Collector->PlayActionSpin();

	const float PresentationSeconds = Collector->GetActionSpinTotalSeconds();
	if (PresentationSeconds <= KINDA_SMALL_NUMBER)
	{
		FinishCollectorActionPresentation();
		return;
	}

	GetWorldTimerManager().SetTimer(
		CollectorActionPresentationTimerHandle,
		this,
		&AShowDownGameModeBase::FinishCollectorActionPresentation,
		PresentationSeconds,
		false);
}

void AShowDownGameModeBase::FinishCollectorActionPresentation()
{
	GetWorldTimerManager().ClearTimer(CollectorActionPresentationTimerHandle);

	bCollectorActionPresentationInProgress = false;

	TFunction<void()> Continuation = MoveTemp(CollectorActionPresentationContinuation);
	CollectorActionPresentationContinuation = TFunction<void()>();
	if (Continuation)
	{
		Continuation();
	}

	if (!bCollectorActionPresentationInProgress && QueuedCollectorActionPresentationContinuations.Num() > 0)
	{
		TFunction<void()> NextContinuation = MoveTemp(QueuedCollectorActionPresentationContinuations[0]);
		QueuedCollectorActionPresentationContinuations.RemoveAt(0);
		PlayCollectorActionPresentationThen(MoveTemp(NextContinuation));
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

	USceneComponent* PlayerHeadSlot = GetPlayerHeadSlot();
	if (!PlayerHeadSlot){
		UE_LOG(LogTemp, Warning, TEXT("Player forehead slot is missing. Add an SDCardPlacementAnchor with PlacementRole=PlayerForehead, or keep an old fallback slot."));
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
	ReflowHandCards(EShowDownSide::Collector);

	PlayerState.ForeheadCard = ChosenCard;
	CurrentRoundCollectorGaveRank = ChosenCard->Rank;
	RecordCurrentRoundAction(FString::Printf(TEXT("Collector gave Player forehead card rank %d."), CurrentRoundCollectorGaveRank));

	// 플레이어는 자기 이마 카드를 보면 안 되므로 false
	CardSystem->MoveCardToSlot(ChosenCard, PlayerHeadSlot, false);

	UE_LOG(LogTemp, Log, TEXT("Collector gave card to player: %s, Rank: %d"), *ChosenCard->GetName(), ChosenCard->Rank);

	WaitForCardPlacementThen(ChosenCard, [this]()
	{
		PlayCollectorActionPresentationThen([this]()
		{
			if (CollectorState.ForeheadCard)
			{
				StartBettingPhase();
			}
			else
			{
				SetPlayerHandSelectable(true);
				if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
				{
					ShowDownGameState->SetPhase(EShowDownPhase::SelectCard);
				}
			}
		});
	});
}

void AShowDownGameModeBase::StartBettingPhase()
{
	const FShowDownStageRule* StageRule = GetCurrentStageRule();
	if (!BettingSystem || !StageRule)
	{
		UE_LOG(LogTemp, Warning, TEXT("BettingSystem is missing on %s."), *GetName());
		return;
	}

	bBettingPhase = true;
	PlayerState.CurrentBet = StageRule->MinimumBet;
	CollectorState.CurrentBet = StageRule->MinimumBet;
	BettingRaisesLeft = 6;
	bHasLastRaiser = false;
	SetPlayerHandSelectable(false);

	BettingSystem->ResetBetting(StageRule->MinimumBet);
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->SetPhase(EShowDownPhase::Betting);
		ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Player, PlayerState.CurrentBet);
		ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Collector, CollectorState.CurrentBet);
	}
	ShowEventDebugMessage(FString::Printf(TEXT("베팅 시작: 기본 %d발"), StageRule->MinimumBet));

	UE_LOG(LogTemp, Log, TEXT("Betting phase started. Stage %d, MinimumBet %d. Q=Check/Call, 1~5=Raise extra bullets, R=Fold"),
		CurrentStageIndex + 1,
		StageRule->MinimumBet);
}

void AShowDownGameModeBase::PlayerCheck()
{
	if (bCollectorActionPresentationInProgress)
	{
		return;
	}

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
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Player, PlayerState.CurrentBet);
		}
		RecordCurrentRoundAction(FString::Printf(TEXT("Player called to %d."), PlayerState.CurrentBet));
		ShowEventDebugMessage(FString::Printf(TEXT("플레이어 콜: %d발"), PlayerState.CurrentBet));
		UE_LOG(LogTemp, Log, TEXT("Player Call %d"), CurrentBet);
		PlayCollectorActionPresentationThen([this]()
		{
			FinishBettingAndResolveRound();
		});
		return;
	}

	BettingSystem->Check(EShowDownSide::Player);
	PlayerState.CurrentBet = CurrentBet;
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Player, PlayerState.CurrentBet);
	}
	RecordCurrentRoundAction(FString::Printf(TEXT("Player checked at %d."), PlayerState.CurrentBet));
	ShowEventDebugMessage(FString::Printf(TEXT("플레이어 체크: %d발"), PlayerState.CurrentBet));
	UE_LOG(LogTemp, Log, TEXT("Player Check"));

	PlayCollectorActionPresentationThen([this]()
	{
		ResolveCollectorBetResponse();
	});
}

void AShowDownGameModeBase::PlayerRaise()
{
	if (!BettingSystem)
	{
		return;
	}

	const int32 CurrentBet = BettingSystem->GetCurrentBet();
	PlayerRaiseTo(CurrentBet + 1);
}

void AShowDownGameModeBase::PlayerRaiseTo(int32 BulletCount)
{
	if (bCollectorActionPresentationInProgress)
	{
		return;
	}

	if (!bBettingPhase || !BettingSystem || !CollectorAISystem)
	{
		return;
	}

	const int32 NewBet = FMath::Clamp(BulletCount, 1, 6);

	if (BettingSystem->RaiseTo(EShowDownSide::Player, NewBet))
	{
		PlayerState.CurrentBet = NewBet;
		BettingRaisesLeft = FMath::Max(0, BettingRaisesLeft - 1);
		bHasLastRaiser = true;
		LastRaiser = EShowDownSide::Player;
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Player, PlayerState.CurrentBet);
		}
		RecordCurrentRoundAction(FString::Printf(TEXT("Player raised to %d."), PlayerState.CurrentBet));
		ShowEventDebugMessage(FString::Printf(TEXT("플레이어 레이즈: %d발"), PlayerState.CurrentBet));
		UE_LOG(LogTemp, Log, TEXT("Player Raise to %d"), NewBet);
		PlayCollectorActionPresentationThen([this]()
		{
			ResolveCollectorBetResponse();
		});
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Player Raise failed. Current bet: %d, requested: %d"),
			BettingSystem->GetCurrentBet(),
			NewBet);
	}
}

void AShowDownGameModeBase::PlayerFold()
{
	if (bCollectorActionPresentationInProgress)
	{
		return;
	}

	if (!bBettingPhase || !BettingSystem)
	{
		return;
	}

	BettingSystem->Fold(EShowDownSide::Player);

	bBettingPhase = false;
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Player, PlayerState.CurrentBet);
	}
	RecordCurrentRoundAction(FString::Printf(TEXT("Player folded at %d."), PlayerState.CurrentBet));
	ShowEventDebugMessage(FString::Printf(TEXT("플레이어 폴드: %d발"), PlayerState.CurrentBet));
	UE_LOG(LogTemp, Log, TEXT("Player Fold"));
	PlayCollectorActionPresentationThen([this]()
	{
		ResolveFold(EShowDownSide::Player);
	});
}

void AShowDownGameModeBase::RequestPlayerBetAction(EShowDownBetAction Action, int32 TargetBet)
{
	switch (Action)
	{
	case EShowDownBetAction::Check:
	case EShowDownBetAction::Call:
		PlayerCheck();
		break;

	case EShowDownBetAction::Raise:
		if (TargetBet > 0)
		{
			PlayerRaiseTo(TargetBet);
		}
		else
		{
			PlayerRaise();
		}
		break;

	case EShowDownBetAction::Fold:
		PlayerFold();
		break;

	default:
		break;
	}
}

void AShowDownGameModeBase::SubmitPlayerDialogueInput(const FString& PlayerDialogue)
{
	SubmitPlayerDialogueInputFromPlayer(PlayerDialogue, TEXT("Player"));
}

void AShowDownGameModeBase::SubmitPlayerDialogueInputFromPlayer(const FString& PlayerDialogue, const FString& SenderName)
{
	const FString TrimmedDialogue = PlayerDialogue.TrimStartAndEnd().Left(240);
	if (TrimmedDialogue.IsEmpty())
	{
		return;
	}

	const FString SafeSenderName = SenderName.TrimStartAndEnd().IsEmpty()
		? TEXT("Player")
		: SenderName.TrimStartAndEnd().Left(32);
	const FString DisplaySenderName = SafeSenderName.StartsWith(TEXT("DESKTOP-"))
		? TEXT("Player")
		: SafeSenderName;

	LatestPlayerDialogueInput = TrimmedDialogue;
	AppendRecentDialogueLine(DisplaySenderName, TrimmedDialogue);
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->BroadcastChatMessage(DisplaySenderName, TrimmedDialogue);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USDLLMSubsystem* LLMSubsystem = GameInstance->GetSubsystem<USDLLMSubsystem>())
		{
			const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			const bool bCooldownReady =
				(CurrentTime - LastBossChatReplyRequestTime) >= LLMSubsystem->BossChatReplyCooldownSeconds;
			const bool bCanRequestInstantReply =
				LLMSubsystem->bEnableInstantBossChatReply
				&& LLMSubsystem->IsConfigured()
				&& bCooldownReady
				&& (LLMSubsystem->bAllowOverlappingBossChatReplies || !bBossChatReplyInFlight);

			if (bCanRequestInstantReply)
			{
				bBossChatReplyInFlight = true;
				LastBossChatReplyRequestTime = CurrentTime;
				if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
				{
					ShowDownGameState->BroadcastCollectorLLMStatus(true, TEXT("Boss is typing..."));
				}

				const FSDLLMBossContext ChatContext = BuildLLMChatContext(TrimmedDialogue);
				LLMSubsystem->RequestBossChatReply(
					ChatContext,
					FSDLLMBossChatCallback::CreateWeakLambda(
						this,
						[this](bool bSuccess, const FString& Dialogue, const FString& Intent)
						{
							bBossChatReplyInFlight = false;
							if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
							{
								if (bSuccess)
								{
									AppendRecentDialogueLine(TEXT("Collector"), Dialogue);
									ShowDownGameState->BroadcastChatMessage(TEXT("Collector"), Dialogue);
									ShowDownGameState->BroadcastCollectorLLMStatus(true, TEXT("Boss answered."));
								}
								else
								{
									ShowDownGameState->BroadcastCollectorLLMStatus(false, TEXT("Boss chat failed."));
								}
							}
						}));
			}
		}
	}
}

void AShowDownGameModeBase::NotifyPresentationFinished(EShowDownPhase FinishedPhase)
{
	EventEnd(FinishedPhase);
}

void AShowDownGameModeBase::EventEnd(EShowDownPhase FinishedPhase)
{
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->EventEnd(FinishedPhase);
	}

	if (FinishedPhase != EShowDownPhase::Reveal)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(RevealDelayHandle);

	if (bHasPendingFoldReveal)
	{
		ContinueFoldAfterReveal(PendingFoldedSide, PendingFoldLoadCount);
		return;
	}

	if (bHasPendingRoundReveal)
	{
		ContinueRoundAfterReveal(PendingRoundResult);
	}
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
	UE_LOG(LogTemp, Log, TEXT("Collector model: given card rank %d, current bet %d, committed %d, raises left %d"),
		GivenCardRank,
		CurrentBet,
		CollectorState.CurrentBet,
		BettingRaisesLeft);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USDLLMSubsystem* LLMSubsystem = GameInstance->GetSubsystem<USDLLMSubsystem>())
		{
			if (LLMSubsystem->IsConfigured())
			{
				if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
				{
					ShowDownGameState->BroadcastCollectorLLMStatus(true, TEXT("Waiting for boss..."));
				}
				const FSDLLMBossContext LLMContext = BuildLLMBossContext(CurrentBet, GivenCardRank);
				LLMSubsystem->RequestBossResponse(
					LLMContext,
					FSDLLMBossResponseCallback::CreateWeakLambda(
						this,
						[this, GivenCardRank](bool bSuccess, const FSDLLMBossResponse& LLMResponse)
						{
							if (!bBettingPhase || !BettingSystem || !CollectorAISystem)
							{
								return;
							}

							const int32 LatestCurrentBet = BettingSystem->GetCurrentBet();
							FCollectorBetDecision CollectorDecision;
							if (bSuccess)
							{
								CollectorDecision = SanitizeCollectorDecision(LLMResponse.Decision);
								UE_LOG(LogTemp, Log, TEXT("Collector LLM decision: %d target %d / dialogue: %s / intent: %s"),
									static_cast<int32>(CollectorDecision.Action),
									CollectorDecision.TargetBet,
									*LLMResponse.Dialogue,
									*LLMResponse.Intent);
								if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
								{
									ShowDownGameState->BroadcastCollectorLLMDecision(
										LLMResponse.Dialogue,
										LLMResponse.Intent,
										CollectorDecision.Action,
										CollectorDecision.TargetBet);
								}
								AppendRecentDialogueLine(TEXT("Collector"), LLMResponse.Dialogue);
							}
							else
							{
								if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
								{
									ShowDownGameState->BroadcastCollectorLLMStatus(false, TEXT("Boss API failed. Using fallback AI."));
								}
								CollectorDecision = CollectorAISystem->ChooseBetDecisionByModel(
									CollectorState.HandCards,
									GivenCardRank,
									LatestCurrentBet,
									CollectorState.CurrentBet,
									6,
									BettingRaisesLeft,
									bHasLastRaiser && LastRaiser == EShowDownSide::Collector);
							}

							ExecuteCollectorBetDecision(CollectorDecision, GivenCardRank);
						}));
				return;
			}

			if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
			{
				ShowDownGameState->BroadcastCollectorLLMStatus(false, TEXT("OPENAI_API_KEY not found. Using fallback AI."));
			}
		}
	}

	const FCollectorBetDecision CollectorDecision = CollectorAISystem->ChooseBetDecisionByModel(
		CollectorState.HandCards,
		GivenCardRank,
		CurrentBet,
		CollectorState.CurrentBet,
		6,
		BettingRaisesLeft,
		bHasLastRaiser && LastRaiser == EShowDownSide::Collector);

	ExecuteCollectorBetDecision(CollectorDecision, GivenCardRank);
}

FCollectorBetDecision AShowDownGameModeBase::SanitizeCollectorDecision(const FCollectorBetDecision& RawDecision) const
{
	FCollectorBetDecision SanitizedDecision = RawDecision;
	if (!BettingSystem)
	{
		return SanitizedDecision;
	}

	const int32 CurrentBet = BettingSystem->GetCurrentBet();
	const bool bMustRespond = CollectorState.CurrentBet < CurrentBet;

	if (SanitizedDecision.Action == EShowDownBetAction::Check && bMustRespond)
	{
		SanitizedDecision.Action = EShowDownBetAction::Call;
	}
	else if (SanitizedDecision.Action == EShowDownBetAction::Call && !bMustRespond)
	{
		SanitizedDecision.Action = EShowDownBetAction::Check;
	}

	if (SanitizedDecision.Action == EShowDownBetAction::Raise)
	{
		if (BettingRaisesLeft <= 0 || CurrentBet >= 6)
		{
			SanitizedDecision.Action = bMustRespond ? EShowDownBetAction::Call : EShowDownBetAction::Check;
			SanitizedDecision.TargetBet = CurrentBet;
		}
		else
		{
			SanitizedDecision.TargetBet = FMath::Clamp(SanitizedDecision.TargetBet, CurrentBet + 1, 6);
		}
	}

	return SanitizedDecision;
}

void AShowDownGameModeBase::ExecuteCollectorBetDecision(const FCollectorBetDecision& CollectorDecision, int32 GivenCardRank)
{
	if (!BettingSystem)
	{
		return;
	}

	const int32 CurrentBet = BettingSystem->GetCurrentBet();
	const EShowDownBetAction CollectorAction = CollectorDecision.Action;

	switch (CollectorAction)
	{
	case EShowDownBetAction::Check:
		BettingSystem->Check(EShowDownSide::Collector);
		CollectorState.CurrentBet = CurrentBet;
		bBettingPhase = false;
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Collector, CollectorState.CurrentBet);
		}
		RecordCurrentRoundAction(FString::Printf(TEXT("Collector checked at %d."), CollectorState.CurrentBet));
		ShowEventDebugMessage(FString::Printf(TEXT("콜렉터 체크: %d발"), CollectorState.CurrentBet));
		UE_LOG(LogTemp, Log, TEXT("Collector Check"));
		PlayCollectorActionPresentationThen([this]()
		{
			FinishBettingAndResolveRound();
		});
		break;

	case EShowDownBetAction::Call:
		BettingSystem->Call(EShowDownSide::Collector);
		CollectorState.CurrentBet = CurrentBet;
		bBettingPhase = false;
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Collector, CollectorState.CurrentBet);
		}
		RecordCurrentRoundAction(FString::Printf(TEXT("Collector called to %d."), CollectorState.CurrentBet));
		ShowEventDebugMessage(FString::Printf(TEXT("콜렉터 콜: %d발"), CollectorState.CurrentBet));
		UE_LOG(LogTemp, Log, TEXT("Collector Call %d"), CurrentBet);
		PlayCollectorActionPresentationThen([this]()
		{
			FinishBettingAndResolveRound();
		});
		break;

	case EShowDownBetAction::Raise:
		{
			const int32 NewBet = FMath::Clamp(CollectorDecision.TargetBet, CurrentBet + 1, 6);
			if (BettingSystem->RaiseTo(EShowDownSide::Collector, NewBet))
			{
				CollectorState.CurrentBet = NewBet;
				BettingRaisesLeft = FMath::Max(0, BettingRaisesLeft - 1);
				bHasLastRaiser = true;
				LastRaiser = EShowDownSide::Collector;
				if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
				{
					ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Collector, CollectorState.CurrentBet);
				}
				RecordCurrentRoundAction(FString::Printf(TEXT("Collector raised to %d."), CollectorState.CurrentBet));
				ShowEventDebugMessage(FString::Printf(TEXT("콜렉터 레이즈: %d발"), CollectorState.CurrentBet));
				UE_LOG(LogTemp, Log, TEXT("Collector Raise to %d"), NewBet);
				UE_LOG(LogTemp, Log, TEXT("Player needs to respond."));
				PlayCollectorActionPresentation();
			}
			break;
		}

	case EShowDownBetAction::Fold:
		BettingSystem->Fold(EShowDownSide::Collector);
		bBettingPhase = false;
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->OnBetChanged.Broadcast(EShowDownSide::Collector, CollectorState.CurrentBet);
		}
		RecordCurrentRoundAction(FString::Printf(TEXT("Collector folded at %d."), CollectorState.CurrentBet));
		ShowEventDebugMessage(FString::Printf(TEXT("콜렉터 폴드: %d발"), CollectorState.CurrentBet));
		UE_LOG(LogTemp, Log, TEXT("Collector Fold"));
		PlayCollectorActionPresentationThen([this]()
		{
			ResolveFold(EShowDownSide::Collector);
		});
		break;

	default:
		break;
	}
}

FSDLLMBossContext AShowDownGameModeBase::BuildLLMBossContext(int32 CurrentBet, int32 GivenCardRank) const
{
	FSDLLMBossContext Context;
	Context.PlayerForeheadRank = GivenCardRank;
	Context.CollectorForeheadRank = CollectorState.ForeheadCard ? CollectorState.ForeheadCard->Rank : 0;
	Context.CurrentBet = CurrentBet;
	Context.PlayerCommittedBet = PlayerState.CurrentBet;
	Context.CollectorCommittedBet = CollectorState.CurrentBet;
	Context.PlayerLives = PlayerState.Lives;
	Context.CollectorLives = CollectorState.Lives;
	Context.RaisesLeft = BettingRaisesLeft;
	Context.Stage = CurrentStageIndex + 1;
	Context.PlayerDialogue = LatestPlayerDialogueInput;
	Context.RecentDialogue = RecentDialogueHistory;
	Context.RecentRoundHistory = RecentRoundHistory;
	Context.CurrentRoundActions = CurrentRoundActionHistory;
	Context.DiscardedCardsSummary = DiscardedCardsSummary.IsEmpty()
		? TEXT("none")
		: DiscardedCardsSummary;
	if (CollectorAISystem)
	{
		Context.CollectorSettings = CollectorAISystem->Settings;
	}

	if (const AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		Context.Round = ShowDownGameState->CurrentRound;
	}

	for (const ACard* Card : CollectorState.HandCards)
	{
		if (Card)
		{
			Context.CollectorHandRanks.Add(Card->Rank);
		}
	}

	return Context;
}

FSDLLMBossContext AShowDownGameModeBase::BuildLLMChatContext(const FString& PlayerDialogue) const
{
	const int32 CurrentBet = BettingSystem ? BettingSystem->GetCurrentBet() : 1;
	const int32 GivenCardRank = PlayerState.ForeheadCard ? PlayerState.ForeheadCard->Rank : 0;
	FSDLLMBossContext Context = BuildLLMBossContext(CurrentBet, GivenCardRank);
	Context.PlayerDialogue = PlayerDialogue;
	Context.RecentDialogue = RecentDialogueHistory;
	return Context;
}

void AShowDownGameModeBase::AppendRecentDialogueLine(const FString& Speaker, const FString& Message)
{
	const FString TrimmedMessage = Message.TrimStartAndEnd();
	if (TrimmedMessage.IsEmpty())
	{
		return;
	}

	if (!RecentDialogueHistory.IsEmpty())
	{
		RecentDialogueHistory += TEXT("\n");
	}
	RecentDialogueHistory += FString::Printf(TEXT("%s: %s"), *Speaker.Left(32), *TrimmedMessage.Left(160));
	RecentDialogueHistory = RecentDialogueHistory.Right(900);
}

void AShowDownGameModeBase::ResetCurrentRoundMemory()
{
	CurrentRoundActionHistory.Reset();
	CurrentRoundPlayerGaveRank = 0;
	CurrentRoundCollectorGaveRank = 0;
	LastRoundPlayerCardRank = 0;
	LastRoundCollectorCardRank = 0;
	bCurrentRoundSummaryRecorded = false;
}

void AShowDownGameModeBase::RecordCurrentRoundAction(const FString& ActionText)
{
	const FString TrimmedAction = ActionText.TrimStartAndEnd();
	if (TrimmedAction.IsEmpty())
	{
		return;
	}

	if (!CurrentRoundActionHistory.IsEmpty())
	{
		CurrentRoundActionHistory += TEXT(" ");
	}
	CurrentRoundActionHistory += TrimmedAction.Left(180);
	CurrentRoundActionHistory = CurrentRoundActionHistory.Right(900);
}

void AShowDownGameModeBase::AppendRecentRoundSummary(EShowDownRoundResult Result, const FString& Reason)
{
	if (bCurrentRoundSummaryRecorded)
	{
		return;
	}

	bCurrentRoundSummaryRecorded = true;

	const int32 RoundNumber = GetShowDownGameState() ? GetShowDownGameState()->CurrentRound : 0;
	const FString SafeActions = CurrentRoundActionHistory.IsEmpty()
		? TEXT("none")
		: CurrentRoundActionHistory;
	const FString Summary = FString::Printf(
		TEXT("Stage %d Round %d: reason=%s, result=%s, player_gave_collector=%d, collector_gave_player=%d, revealed_player=%d, revealed_collector=%d, player_bet=%d, collector_bet=%d, player_lives=%d, collector_lives=%d, actions=[%s]."),
		CurrentStageIndex + 1,
		RoundNumber,
		*Reason.Left(64),
		*GetRoundResultText(Result),
		CurrentRoundPlayerGaveRank,
		CurrentRoundCollectorGaveRank,
		LastRoundPlayerCardRank,
		LastRoundCollectorCardRank,
		PlayerState.CurrentBet,
		CollectorState.CurrentBet,
		PlayerState.Lives,
		CollectorState.Lives,
		*SafeActions.Left(700));

	if (!RecentRoundHistory.IsEmpty())
	{
		RecentRoundHistory += TEXT("\n");
	}
	RecentRoundHistory += Summary;
	RecentRoundHistory = RecentRoundHistory.Right(1400);
}

FString AShowDownGameModeBase::GetSideText(EShowDownSide Side) const
{
	return Side == EShowDownSide::Player ? TEXT("Player") : TEXT("Collector");
}

FString AShowDownGameModeBase::GetSideDisplayText(EShowDownSide Side) const
{
	return Side == EShowDownSide::Player ? TEXT("플레이어") : TEXT("콜렉터");
}

FString AShowDownGameModeBase::GetRoundResultText(EShowDownRoundResult Result) const
{
	switch (Result)
	{
	case EShowDownRoundResult::PlayerWin:
		return TEXT("PlayerWin");

	case EShowDownRoundResult::CollectorWin:
		return TEXT("CollectorWin");

	case EShowDownRoundResult::Draw:
		return TEXT("Draw");

	default:
		return TEXT("Unknown");
	}
}

void AShowDownGameModeBase::BeginCardSelectionRound()
{
	ResetCurrentRoundMemory();
	CurrentRoundFirstSide = NextRoundFirstSide;

	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->SetPhase(EShowDownPhase::SelectCard);
	}

	if (CurrentRoundFirstSide == EShowDownSide::Collector)
	{
		SetPlayerHandSelectable(false);
		CollectorGiveCardToPlayer();
		return;
	}

	SetPlayerHandSelectable(true);
}

void AShowDownGameModeBase::SetNextRoundFirstSideFromResult(EShowDownRoundResult Result)
{
	switch (Result)
	{
	case EShowDownRoundResult::PlayerWin:
		NextRoundFirstSide = EShowDownSide::Collector;
		break;

	case EShowDownRoundResult::CollectorWin:
		NextRoundFirstSide = EShowDownSide::Player;
		break;

	case EShowDownRoundResult::Draw:
	default:
		NextRoundFirstSide = CurrentRoundFirstSide;
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
	LastRoundPlayerCardRank = PlayerCardRank;
	LastRoundCollectorCardRank = CollectorCardRank;
	const EShowDownRoundResult Result = RoundResolver->ResolveRevealedCards(PlayerCardRank, CollectorCardRank);
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->SetPhase(EShowDownPhase::Reveal);
		ShowDownGameState->OnCardsRevealed.Broadcast(PlayerCardRank, CollectorCardRank);
		ShowDownGameState->OnRoundResolved.Broadcast(Result);
	}
	ShowEventDebugMessage(FString::Printf(TEXT("승부 공개: 플레이어 %d / 콜렉터 %d"),
		PlayerCardRank,
		CollectorCardRank));
	UE_LOG(LogTemp, Log, TEXT("Reveal cards. Player: %d, Collector: %d"), PlayerCardRank, CollectorCardRank);
	UE_LOG(LogTemp, Log, TEXT("Reveal resolved. Waiting for presentation or auto-advance fallback."));

	bHasPendingRoundReveal = true;
	bHasPendingFoldReveal = false;
	PendingRoundResult = Result;

	PlayCollectorActionPresentationThen([this]()
	{
		ScheduleRevealAutoAdvanceIfNeeded();
	});
}

void AShowDownGameModeBase::ContinueRoundAfterReveal(EShowDownRoundResult Result)
{
	bHasPendingRoundReveal = false;
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->EventEnd(EShowDownPhase::Reveal);
	}

	// 결과 공개 직후 보스가 승/패/무에 대한 반응 채팅을 남긴다.
	BroadcastBossResultReaction(Result);

	SetNextRoundFirstSideFromResult(Result);

	const FString ResultDisplayText = Result == EShowDownRoundResult::PlayerWin
		? TEXT("플레이어 승리")
		: Result == EShowDownRoundResult::CollectorWin
			? TEXT("콜렉터 승리")
			: TEXT("무승부");
	ShowEventDebugMessage(FString::Printf(TEXT("결과: %s / 다음 선공: %s"),
		*ResultDisplayText,
		*GetSideDisplayText(NextRoundFirstSide)));

	switch (Result)
	{
	case EShowDownRoundResult::PlayerWin:
		UE_LOG(LogTemp, Log, TEXT("Round result: Player wins. Collector roulette with %d bullet(s)."), CollectorState.CurrentBet);
		ApplyRouletteResult(EShowDownSide::Collector, CollectorState.CurrentBet, [this, Result]()
		{
			AppendRecentRoundSummary(Result, TEXT("card reveal"));
			EndRound();
		});
		break;

	case EShowDownRoundResult::CollectorWin:
		UE_LOG(LogTemp, Log, TEXT("Round result: Collector wins. Player roulette with %d bullet(s)."), PlayerState.CurrentBet);
		ApplyRouletteResult(EShowDownSide::Player, PlayerState.CurrentBet, [this, Result]()
		{
			AppendRecentRoundSummary(Result, TEXT("card reveal"));
			EndRound();
		});
		break;

	case EShowDownRoundResult::Draw:
		UE_LOG(LogTemp, Log, TEXT("Round result: Draw. Both sides roulette."));
		ApplyRouletteResult(EShowDownSide::Collector, CollectorState.CurrentBet, [this, Result]()
		{
			ApplyRouletteResult(EShowDownSide::Player, PlayerState.CurrentBet, [this, Result]()
			{
				AppendRecentRoundSummary(Result, TEXT("card reveal"));
				EndRound();
			});
		});
		break;

	default:
		break;
	}
}

void AShowDownGameModeBase::BroadcastBossResultReaction(EShowDownRoundResult Result)
{
	AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState();
	if (!ShowDownGameState)
	{
		return;
	}

	// Collector(보스) 관점의 결과와 LLM 실패 시 쓸 정적 폴백 대사
	FString Outcome;
	FString FallbackLine;
	switch (Result)
	{
	case EShowDownRoundResult::PlayerWin: // 보스 패배
		Outcome = TEXT("collector_lost");
		FallbackLine = TEXT("아쉽군… 다음엔 다르다.");
		break;
	case EShowDownRoundResult::CollectorWin: // 보스 승리
		Outcome = TEXT("collector_won");
		FallbackLine = TEXT("예상대로군.");
		break;
	case EShowDownRoundResult::Draw:
	default:
		Outcome = TEXT("draw");
		FallbackLine = TEXT("무승부라… 시시하군.");
		break;
	}

	UGameInstance* GameInstance = GetGameInstance();
	USDLLMSubsystem* LLMSubsystem = GameInstance ? GameInstance->GetSubsystem<USDLLMSubsystem>() : nullptr;
	if (!LLMSubsystem || !LLMSubsystem->IsConfigured())
	{
		// LLM 비활성/미설정 → 정적 대사로 대체
		AppendRecentDialogueLine(TEXT("Collector"), FallbackLine);
		ShowDownGameState->BroadcastChatMessage(TEXT("Collector"), FallbackLine);
		return;
	}

	const int32 CurrentBet = BettingSystem ? BettingSystem->GetCurrentBet() : 1;
	const int32 GivenCardRank = PlayerState.ForeheadCard ? PlayerState.ForeheadCard->Rank : 0;
	FSDLLMBossContext Context = BuildLLMBossContext(CurrentBet, GivenCardRank);
	Context.RoundOutcome = Outcome;

	LLMSubsystem->RequestBossResultReaction(
		Context,
		FSDLLMBossChatCallback::CreateWeakLambda(
			this,
			[this, FallbackLine](bool bSuccess, const FString& Dialogue, const FString& Intent)
			{
				AShowDownGameStateBase* CallbackGameState = GetShowDownGameState();
				if (!CallbackGameState)
				{
					return;
				}

				const FString Line = (bSuccess && !Dialogue.TrimStartAndEnd().IsEmpty())
					? Dialogue
					: FallbackLine;
				AppendRecentDialogueLine(TEXT("Collector"), Line);
				CallbackGameState->BroadcastChatMessage(TEXT("Collector"), Line);
			}));
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
	const FShowDownStageRule* StageRule = GetCurrentStageRule();
	const bool bSevenFoldLoadsSix = StageRule ? StageRule->bSevenFoldLoadsSix : true;
	const int32 LoadCount = RoundResolver->GetFoldLoadCount(FoldedCardRank, FoldedState.CurrentBet, bSevenFoldLoadsSix);

	if (PlayerState.ForeheadCard)
	{
		PlayerState.ForeheadCard->SetFaceUp(true);
	}

	if (CollectorState.ForeheadCard)
	{
		CollectorState.ForeheadCard->SetFaceUp(true);
	}

	const int32 PlayerCardRank = PlayerState.ForeheadCard ? PlayerState.ForeheadCard->Rank : 0;
	const int32 CollectorCardRank = CollectorState.ForeheadCard ? CollectorState.ForeheadCard->Rank : 0;
	LastRoundPlayerCardRank = PlayerCardRank;
	LastRoundCollectorCardRank = CollectorCardRank;

	const EShowDownRoundResult FoldResult = FoldedSide == EShowDownSide::Player
		? EShowDownRoundResult::CollectorWin
		: EShowDownRoundResult::PlayerWin;
	SetNextRoundFirstSideFromResult(FoldResult);

	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->SetPhase(EShowDownPhase::Reveal);
		ShowDownGameState->OnCardsRevealed.Broadcast(PlayerCardRank, CollectorCardRank);
		ShowDownGameState->OnRoundResolved.Broadcast(FoldResult);
	}
	ShowEventDebugMessage(FString::Printf(TEXT("%s 폴드: 플레이어 %d / 콜렉터 %d / 다음 선공: %s"),
		*GetSideDisplayText(FoldedSide),
		PlayerCardRank,
		CollectorCardRank,
		*GetSideDisplayText(NextRoundFirstSide)));
	UE_LOG(LogTemp, Log, TEXT("%s folded. Forehead card: %d, roulette load: %d"),
		FoldedSide == EShowDownSide::Player ? TEXT("Player") : TEXT("Collector"),
		FoldedCardRank,
		LoadCount);
	UE_LOG(LogTemp, Log, TEXT("Fold reveal resolved. Waiting for presentation or auto-advance fallback."));

	bHasPendingRoundReveal = false;
	bHasPendingFoldReveal = true;
	PendingFoldedSide = FoldedSide;
	PendingFoldLoadCount = LoadCount;

	PlayCollectorActionPresentationThen([this]()
	{
		ScheduleRevealAutoAdvanceIfNeeded();
	});
}

void AShowDownGameModeBase::ContinueFoldAfterReveal(EShowDownSide FoldedSide, int32 LoadCount)
{
	bHasPendingFoldReveal = false;
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->EventEnd(EShowDownPhase::Reveal);
	}

	// 폴드로 끝난 라운드도 보스 반응 채팅을 남긴다.
	BroadcastBossResultReaction(
		FoldedSide == EShowDownSide::Player ? EShowDownRoundResult::CollectorWin : EShowDownRoundResult::PlayerWin);

	ApplyRouletteResult(FoldedSide, LoadCount, [this, FoldedSide]()
	{
		AppendRecentRoundSummary(
			FoldedSide == EShowDownSide::Player ? EShowDownRoundResult::CollectorWin : EShowDownRoundResult::PlayerWin,
			FString::Printf(TEXT("%s folded"), *GetSideText(FoldedSide)));
		EndRound();
	});
}

void AShowDownGameModeBase::ApplyRouletteResult(EShowDownSide TargetSide, int32 BulletCount, TFunction<void()>&& Continuation)
{
	if (!RouletteSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot roll roulette. RouletteSystem is missing."));
		if (Continuation)
		{
			Continuation();
		}
		return;
	}

	const int32 ClampedBulletCount = FMath::Clamp(BulletCount, 1, 6);
	const float HitChance = RouletteSystem->GetHitChance(ClampedBulletCount);
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->SetPhase(EShowDownPhase::Roulette);
		ShowDownGameState->OnRouletteStarted.Broadcast(TargetSide, ClampedBulletCount);
	}
	const bool bHit = RouletteSystem->RollRoulette(ClampedBulletCount);
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->OnRouletteResult.Broadcast(TargetSide, bHit);
	}

	UE_LOG(LogTemp, Log, TEXT("%s roulette: %d/6, %.0f%% chance, result: %s"),
		TargetSide == EShowDownSide::Player ? TEXT("Player") : TEXT("Collector"),
		ClampedBulletCount,
		HitChance * 100.0f,
		bHit ? TEXT("Hit") : TEXT("Miss"));

	if (!bHit)
	{
		ShowEventDebugMessage(FString::Printf(TEXT("룰렛: %s %d발 / 안 맞음"),
			*GetSideDisplayText(TargetSide),
			ClampedBulletCount));
		PlayCollectorActionPresentationThen(MoveTemp(Continuation));
		return;
	}

	FShowDownParticipantState& TargetState = TargetSide == EShowDownSide::Player ? PlayerState : CollectorState;
	TargetState.Lives = FMath::Max(0, TargetState.Lives - 1);
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->OnLifeChanged.Broadcast(TargetSide, TargetState.Lives);
	}
	ShowEventDebugMessage(FString::Printf(TEXT("룰렛: %s %d발 / 총 맞음 / 목숨 %d"),
		*GetSideDisplayText(TargetSide),
		ClampedBulletCount,
		TargetState.Lives));
	PlayCollectorActionPresentationThen(MoveTemp(Continuation));

	UE_LOG(LogTemp, Log, TEXT("%s lives: %d"),
		TargetSide == EShowDownSide::Player ? TEXT("Player") : TEXT("Collector"),
		TargetState.Lives);
}

void AShowDownGameModeBase::EndRound()
{
	bBettingPhase = false;
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->SetPhase(EShowDownPhase::RoundEnd);
	}

	ClearForeheadCards();

	if (PlayerState.Lives <= 0)
	{
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->SetPhase(EShowDownPhase::GameOver);
			ShowDownGameState->OnGameOver.Broadcast(EShowDownSide::Collector);
		}
		ShowEventDebugMessage(TEXT("게임 종료 - 콜렉터 승리"));
		UE_LOG(LogTemp, Log, TEXT("Game Over. Player has no lives left."));
		return;
	}

	if (CollectorState.Lives <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Stage %d cleared. Collector has no lives left."), CurrentStageIndex + 1);
		AdvanceStage();
		return;
	}

	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->CurrentRound++;
	}

	const bool bNeedRedeal = PlayerState.HandCards.Num() <= 0 || CollectorState.HandCards.Num() <= 0;
	if (bNeedRedeal)
	{
		UE_LOG(LogTemp, Log, TEXT("Hands are empty. Shuffling and dealing new 5-card hands."));
		DealInitialHand();
		BeginCardSelectionRound();
		return;
	}

	BeginCardSelectionRound();
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
	for (int32 Rank : DiscardRanks)
	{
		if (!DiscardedCardsSummary.IsEmpty())
		{
			DiscardedCardsSummary += TEXT(", ");
		}
		DiscardedCardsSummary += FString::FromInt(Rank);
	}
	DiscardedCardsSummary = DiscardedCardsSummary.Right(240);
}

void AShowDownGameModeBase::ClearHandCards()
{
	for (ACard* Card : PlayerState.HandCards)
	{
		if (Card)
		{
			Card->Destroy();
		}
	}
	PlayerState.HandCards.Reset();

	for (ACard* Card : CollectorState.HandCards)
	{
		if (Card)
		{
			Card->Destroy();
		}
	}
	CollectorState.HandCards.Reset();
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

FSDCardHandLayoutSettings AShowDownGameModeBase::GetDefaultHandLayoutSettings() const
{
	FSDCardHandLayoutSettings Settings;
	Settings.CardSpacing = CardSpacing;
	Settings.ForwardOffset = ForwardOffset;
	Settings.HeightOffset = HeightOffset;
	Settings.LeanAngle = LeanAngle;
	Settings.LayerStep = LayerStep;
	return Settings;
}

FSDCardHandLayoutSettings AShowDownGameModeBase::ResolveHandLayoutSettings(EShowDownSide Side) const
{
	FSDCardHandLayoutSettings Settings = GetDefaultHandLayoutSettings();

	if (const ASDCardPlacementAnchor* HandAnchor = GetHandAnchorForSide(Side))
	{
		Settings.CardSpacing = HandAnchor->CardSpacing;
		Settings.ForwardOffset = HandAnchor->ForwardOffset;
		Settings.HeightOffset = HandAnchor->HeightOffset;
		Settings.LeanAngle = HandAnchor->LeanAngle;
		Settings.LayerStep = HandAnchor->LayerStep;
		return Settings;
	}

	if (const ASDPlayerSeat* Seat = GetSeatForSide(Side))
	{
		if (Seat->bOverrideGameModeHandLayout)
		{
			Settings.CardSpacing = Seat->HandSpacing;
			Settings.ForwardOffset = Seat->HandForwardOffset;
			Settings.HeightOffset = Seat->HandHeightOffset;
			Settings.LeanAngle = Seat->HandLeanAngle;
		}
	}

	return Settings;
}

void AShowDownGameModeBase::ApplyCardMotionForSide(EShowDownSide Side, const TArray<ACard*>& Cards) const
{
	const ASDCardPlacementAnchor* HandAnchor = GetHandAnchorForSide(Side);
	const ASDPlayerSeat* Seat = GetSeatForSide(Side);
	if (!HandAnchor && (!Seat || !Seat->bOverrideCardMotion))
	{
		return;
	}

	for (ACard* Card : Cards)
	{
		if (!Card)
		{
			continue;
		}

		Card->SelectedOffset = HandAnchor ? HandAnchor->SelectedOffset : Seat->SelectedOffset;
		Card->HoverOffset = HandAnchor ? HandAnchor->HoverOffset : Seat->HoverOffset;
		Card->MoveSpeed = HandAnchor ? HandAnchor->MoveSpeed : Seat->MoveSpeed;
	}
}

void AShowDownGameModeBase::ReflowHandCards(EShowDownSide Side)
{
	if (!CardSystem)
	{
		return;
	}

	USceneComponent* HandSlot = GetHandSlotForSide(Side);
	if (!HandSlot)
	{
		return;
	}

	const TArray<ACard*>& Cards = Side == EShowDownSide::Collector
		? CollectorState.HandCards
		: PlayerState.HandCards;

	CardSystem->LayoutHandCards(this, HandSlot, ResolveHandLayoutSettings(Side), Cards);
	ApplyCardMotionForSide(Side, Cards);
}

void AShowDownGameModeBase::RefreshNetworkPlayerSlots()
{
	AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState();
	if (!ShowDownGameState)
	{
		return;
	}

	ShowDownGameState->SetMatchMode(GetNetMode() == NM_Standalone
		? EShowDownMatchMode::SinglePlayer
		: EShowDownMatchMode::Multiplayer);

	TSet<EShowDownPlayerSlot> SeenSlots;
	if (GameState)
	{
		for (APlayerState* BasePlayerState : GameState->PlayerArray)
		{
			ASDPlayerState* ShowDownPlayerState = Cast<ASDPlayerState>(BasePlayerState);
			if (!ShowDownPlayerState)
			{
				continue;
			}

			if (ShowDownPlayerState->ShowDownSlot == EShowDownPlayerSlot::None)
			{
				ShowDownPlayerState->SetShowDownSlot(FindNextOpenPlayerSlot());
			}

			FShowDownNetworkPlayerSlot SlotState;
			SlotState.Slot = ShowDownPlayerState->ShowDownSlot;
			SlotState.PlayerId = FString::FromInt(ShowDownPlayerState->GetPlayerId());
			SlotState.DisplayName = ShowDownPlayerState->GetPlayerName();
			SlotState.bConnected = true;
			SlotState.bReady = ShowDownPlayerState->bReady;

			ShowDownGameState->SetPlayerSlot(SlotState);
			SeenSlots.Add(SlotState.Slot);
		}
	}

	const EShowDownPlayerSlot AllSlots[] = {
		EShowDownPlayerSlot::Player1,
		EShowDownPlayerSlot::Player2,
		EShowDownPlayerSlot::Player3,
		EShowDownPlayerSlot::Player4
	};

	for (EShowDownPlayerSlot Slot : AllSlots)
	{
		if (!SeenSlots.Contains(Slot))
		{
			ShowDownGameState->ClearPlayerSlot(Slot);
		}
	}
}

EShowDownPlayerSlot AShowDownGameModeBase::FindNextOpenPlayerSlot() const
{
	TSet<EShowDownPlayerSlot> UsedSlots;
	if (GameState)
	{
		for (APlayerState* BasePlayerState : GameState->PlayerArray)
		{
			if (const ASDPlayerState* ShowDownPlayerState = Cast<ASDPlayerState>(BasePlayerState))
			{
				UsedSlots.Add(ShowDownPlayerState->ShowDownSlot);
			}
		}
	}

	const EShowDownPlayerSlot AllSlots[] = {
		EShowDownPlayerSlot::Player1,
		EShowDownPlayerSlot::Player2,
		EShowDownPlayerSlot::Player3,
		EShowDownPlayerSlot::Player4
	};

	for (EShowDownPlayerSlot Slot : AllSlots)
	{
		if (!UsedSlots.Contains(Slot))
		{
			return Slot;
		}
	}

	return EShowDownPlayerSlot::None;
}

void AShowDownGameModeBase::StartStage(int32 StageIndex)
{
	if (!StageRules.IsValidIndex(StageIndex))
	{
		if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
		{
			ShowDownGameState->SetPhase(EShowDownPhase::GameOver);
			ShowDownGameState->OnGameOver.Broadcast(EShowDownSide::Player);
		}
		ShowEventDebugMessage(TEXT("게임 종료 - 플레이어 승리"));
		UE_LOG(LogTemp, Log, TEXT("All stages cleared. Player wins the game."));
		return;
	}

	CurrentStageIndex = StageIndex;
	const FShowDownStageRule& StageRule = StageRules[CurrentStageIndex];

	bBettingPhase = false;
	GetWorldTimerManager().ClearTimer(RevealDelayHandle);

	ClearForeheadCards();
	ClearHandCards();

	PlayerState.Lives = StageRule.StartingLives;
	CollectorState.Lives = StageRule.StartingLives;
	PlayerState.CurrentBet = StageRule.MinimumBet;
	CollectorState.CurrentBet = StageRule.MinimumBet;
	BettingRaisesLeft = 6;
	bHasLastRaiser = false;
	CurrentRoundFirstSide = EShowDownSide::Player;
	NextRoundFirstSide = EShowDownSide::Player;
	RecentRoundHistory.Reset();
	CurrentRoundActionHistory.Reset();
	DiscardedCardsSummary.Reset();
	RecentDialogueHistory.Reset();
	ResetCurrentRoundMemory();

	if (CollectorAISystem)
	{
		CollectorAISystem->Settings.BluffRate = StageRule.CollectorBluffRate;
		CollectorAISystem->Settings.Aggression = StageRule.CollectorAggression;
	}
	if (AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState())
	{
		ShowDownGameState->CurrentStage = CurrentStageIndex + 1;
		ShowDownGameState->CurrentRound = 1;
		ShowDownGameState->OnStageChanged.Broadcast(CurrentStageIndex + 1);
	}

	DealInitialHand();
	ShowEventDebugMessage(FString::Printf(TEXT("스테이지 %d 시작"), CurrentStageIndex + 1));
	BeginCardSelectionRound();

	UE_LOG(LogTemp, Log, TEXT("Stage %d started. Lives: %d, MinBet: %d, Collector Bluff: %.2f, Aggression: %.2f"),
		CurrentStageIndex + 1,
		StageRule.StartingLives,
		StageRule.MinimumBet,
		StageRule.CollectorBluffRate,
		StageRule.CollectorAggression);
}

void AShowDownGameModeBase::AdvanceStage()
{
	StartStage(CurrentStageIndex + 1);
}

const FShowDownStageRule* AShowDownGameModeBase::GetCurrentStageRule() const
{
	return StageRules.IsValidIndex(CurrentStageIndex) ? &StageRules[CurrentStageIndex] : nullptr;
}

AShowDownGameStateBase* AShowDownGameModeBase::GetShowDownGameState() const
{
	return GetGameState<AShowDownGameStateBase>();
}

APlayerPawn* AShowDownGameModeBase::GetPrimaryPlayerPawn() const
{
	return Cast<APlayerPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

ASDCardPlacementAnchor* AShowDownGameModeBase::GetCardPlacementAnchor(EShowDownSide Side, bool bForeheadSlot) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const ESDCardPlacementRole TargetRole = Side == EShowDownSide::Collector
		? (bForeheadSlot ? ESDCardPlacementRole::OpponentForehead : ESDCardPlacementRole::OpponentHand)
		: (bForeheadSlot ? ESDCardPlacementRole::PlayerForehead : ESDCardPlacementRole::PlayerHand);

	TArray<AActor*> AnchorActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASDCardPlacementAnchor::StaticClass(), AnchorActors);

	for (AActor* AnchorActor : AnchorActors)
	{
		ASDCardPlacementAnchor* Anchor = Cast<ASDCardPlacementAnchor>(AnchorActor);
		if (Anchor && Anchor->PlacementRole == TargetRole)
		{
			return Anchor;
		}
	}

	return nullptr;
}

ASDCardPlacementAnchor* AShowDownGameModeBase::GetHandAnchorForSide(EShowDownSide Side) const
{
	return GetCardPlacementAnchor(Side, false);
}

ASDCardPlacementAnchor* AShowDownGameModeBase::GetForeheadAnchorForSide(EShowDownSide Side) const
{
	return GetCardPlacementAnchor(Side, true);
}

ASDPlayerSeat* AShowDownGameModeBase::GetSeatForSide(EShowDownSide Side) const
{
	TArray<AActor*> SeatActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDPlayerSeat::StaticClass(), SeatActors);

	ASDPlayerSeat* FirstSeat = nullptr;
	for (AActor* SeatActor : SeatActors)
	{
		ASDPlayerSeat* Seat = Cast<ASDPlayerSeat>(SeatActor);
		if (!Seat)
		{
			continue;
		}

		if (!FirstSeat)
		{
			FirstSeat = Seat;
		}

		if (Seat->SeatSide == Side)
		{
			return Seat;
		}
	}

	// Old maps may have a single SDPlayerSeat without an explicit side set yet.
	return Side == EShowDownSide::Player ? FirstSeat : nullptr;
}

ASDPlayerSeat* AShowDownGameModeBase::GetPrimaryPlayerSeat() const
{
	return GetSeatForSide(EShowDownSide::Player);
}

USceneComponent* AShowDownGameModeBase::GetHandSlotForSide(EShowDownSide Side) const
{
	if (const ASDCardPlacementAnchor* Anchor = GetHandAnchorForSide(Side))
	{
		if (USceneComponent* HandSlot = Anchor->GetSlotComponent())
		{
			return HandSlot;
		}
	}

	if (const ASDPlayerSeat* Seat = GetSeatForSide(Side))
	{
		if (USceneComponent* HandSlot = Seat->GetHandSlot())
		{
			return HandSlot;
		}
	}

	if (Side == EShowDownSide::Player)
	{
		if (const APlayerPawn* PlayerPawn = GetPrimaryPlayerPawn())
		{
			return PlayerPawn->PlayerHandCard;
		}
	}

	if (Side == EShowDownSide::Collector && Collector)
	{
		return Collector->c_HandCard;
	}

	return nullptr;
}

USceneComponent* AShowDownGameModeBase::GetHeadSlotForSide(EShowDownSide Side) const
{
	if (const ASDCardPlacementAnchor* Anchor = GetForeheadAnchorForSide(Side))
	{
		if (USceneComponent* HeadSlot = Anchor->GetSlotComponent())
		{
			return HeadSlot;
		}
	}

	if (const ASDPlayerSeat* Seat = GetSeatForSide(Side))
	{
		if (USceneComponent* HeadSlot = Seat->GetHeadSlot())
		{
			return HeadSlot;
		}
	}

	if (Side == EShowDownSide::Player)
	{
		if (const APlayerPawn* PlayerPawn = GetPrimaryPlayerPawn())
		{
			return PlayerPawn->PlayerHeadCard;
		}
	}

	if (Side == EShowDownSide::Collector && Collector)
	{
		return Collector->c_HeadCard;
	}

	return nullptr;
}

USceneComponent* AShowDownGameModeBase::GetPlayerHandSlot() const
{
	return GetHandSlotForSide(EShowDownSide::Player);
}

USceneComponent* AShowDownGameModeBase::GetPlayerHeadSlot() const
{
	return GetHeadSlotForSide(EShowDownSide::Player);
}

void AShowDownGameModeBase::ScheduleRevealAutoAdvanceIfNeeded()
{
	GetWorldTimerManager().ClearTimer(RevealDelayHandle);

	const AShowDownGameStateBase* ShowDownGameState = GetShowDownGameState();
	const bool bPresentationIsHandled = ShowDownGameState && ShowDownGameState->OnPresentationStarted.IsBound();
	if (bPresentationIsHandled || !bAutoAdvanceRevealWithoutPresentation)
	{
		return;
	}

	if (RevealAutoAdvanceSeconds <= 0.0f)
	{
		EventEnd(EShowDownPhase::Reveal);
		return;
	}

	GetWorldTimerManager().SetTimer(
		RevealDelayHandle,
		FTimerDelegate::CreateUObject(this, &AShowDownGameModeBase::EventEnd, EShowDownPhase::Reveal),
		RevealAutoAdvanceSeconds,
		false);
}

void AShowDownGameModeBase::ShowEventDebugMessage(const FString& Message) const
{
	if (bShowGameFlowDebugMessages && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.8f, FColor::Cyan, FString::Printf(TEXT("[게임] %s"), *Message));
	}
}
