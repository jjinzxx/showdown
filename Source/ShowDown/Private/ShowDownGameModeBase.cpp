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

	UE_LOG(LogTemp, Log, TEXT("GameMode received selected card: %s"), *SelectedCard->GetName());

	RemoveHandCard(PlayerState, SelectedCard);
	//콜렉터의 이마로 카드 이동
	CollectorState.ForeheadCard = SelectedCard;
	PlaceCardOnCollectorHead(SelectedCard);
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

	SpawnHandCards(PlayerState, PlayerPawn->PlayerHandCard, PlayerRanks, true);

	// 확인용으로 true. 나중에는 false로 바꾸면 콜렉터 손패가 뒷면이 됩니다.
	SpawnHandCards(CollectorState, Collector->c_HandCard, CollectorRanks, true);

	UE_LOG(LogTemp, Log, TEXT("Player hand count: %d"), PlayerState.HandCards.Num());
	UE_LOG(LogTemp, Log, TEXT("Collector hand count: %d"), CollectorState.HandCards.Num());
}

void AShowDownGameModeBase::AddHandCard(FShowDownParticipantState& Participant, ACard* NewCard)
{
	if (!NewCard)
	{
		return;
	}

	Participant.HandCards.Add(NewCard);
}

void AShowDownGameModeBase::RemoveHandCard(FShowDownParticipantState& Participant, ACard* RemovedCard)
{
	if (!RemovedCard)
	{
		return;
	}

	Participant.HandCards.Remove(RemovedCard);
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

void AShowDownGameModeBase::PlaceCardOnCollectorHead(ACard* SelectedCard)
{
	if (!SelectedCard)
	{
		return;
	}

	if (!Collector || !Collector->c_HeadCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Collector or Collector head card slot is missing."));
		return;
	}

	SelectedCard->MoveToSlot(Collector->c_HeadCard, true);
}
void AShowDownGameModeBase::SpawnHandCards(
	FShowDownParticipantState& Participant,
	USceneComponent* HandRoot,
	const TArray<int32>& Ranks,
	bool bFaceUp)
{
	if (!HandRoot)
	{
		return;
	}
	
	Participant.HandCards.Reset();
	
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

		ACard* NewCard = GetWorld()->SpawnActor<ACard>(CardClass, SpawnLocation, SpawnRotation);
		if (!NewCard)
		{
			continue;
		}

		NewCard->SetCard(Rank);
		NewCard->SetFaceUp(bFaceUp);

		AddHandCard(Participant, NewCard);
	}
}
	
