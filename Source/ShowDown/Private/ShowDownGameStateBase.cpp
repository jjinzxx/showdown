#include "ShowDownGameStateBase.h"

#include "Engine/Engine.h"

namespace
{
	FString GetPhaseDebugName(EShowDownPhase Phase)
	{
		switch (Phase)
		{
		case EShowDownPhase::SelectCard:
			return TEXT("카드 선택");
		case EShowDownPhase::Betting:
			return TEXT("베팅");
		case EShowDownPhase::Reveal:
			return TEXT("카드 공개");
		case EShowDownPhase::Roulette:
			return TEXT("룰렛");
		case EShowDownPhase::RoundEnd:
			return TEXT("라운드 종료");
		case EShowDownPhase::GameOver:
			return TEXT("게임 종료");
		case EShowDownPhase::None:
		default:
			return TEXT("없음");
		}
	}

	void ShowPresentationDebugMessage(const FString& Prefix, EShowDownPhase Phase, const FColor& Color)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				4.0f,
				Color,
				FString::Printf(TEXT("[%s] %s 연출"), *Prefix, *GetPhaseDebugName(Phase)));
		}
	}

	void ShowRawDebugMessage(const FString& Message, const FColor& Color)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, Color, Message);
		}
	}
}

void AShowDownGameStateBase::SetPhase(EShowDownPhase NewPhase)
{
	if (CurrentPhase == NewPhase)
	{
		return;
	}

	const EShowDownPhase PhaseToFinish = CurrentPresentationPhase != EShowDownPhase::None
		? CurrentPresentationPhase
		: CurrentPhase;
	EventEnd(PhaseToFinish);

	CurrentPhase = NewPhase;
	OnPhaseChanged.Broadcast(CurrentPhase);
	EventStart(CurrentPhase);
}

void AShowDownGameStateBase::EventStart(EShowDownPhase Phase)
{
	if (Phase == EShowDownPhase::None)
	{
		return;
	}

	CurrentPresentationPhase = Phase;
	bPresentationPlaying = true;
	ShowPresentationDebugMessage(TEXT("호출"), Phase, FColor::Orange);

	const bool bHasPresentationEvent = OnPresentationStarted.IsBound();
	OnPresentationStarted.Broadcast(Phase);

	if (!bHasPresentationEvent)
	{
		ShowRawDebugMessage(TEXT("*"), FColor::White);
		EventEnd(Phase);
	}
}

void AShowDownGameStateBase::EventEnd(EShowDownPhase Phase)
{
	if (Phase == EShowDownPhase::None || !bPresentationPlaying)
	{
		return;
	}

	const EShowDownPhase FinishedPhase = CurrentPresentationPhase != EShowDownPhase::None
		? CurrentPresentationPhase
		: Phase;

	bPresentationPlaying = false;
	OnPresentationFinished.Broadcast(FinishedPhase);
	ShowPresentationDebugMessage(TEXT("호출종료"), FinishedPhase, FColor::Green);
	CurrentPresentationPhase = EShowDownPhase::None;
}

void AShowDownGameStateBase::StartPresentation(EShowDownPhase Phase)
{
	EventStart(Phase);
}

void AShowDownGameStateBase::FinishPresentation(EShowDownPhase Phase)
{
	EventEnd(Phase);
}

void AShowDownGameStateBase::BroadcastCollectorDialogue(const FString& Dialogue, const FString& Intent)
{
	OnCollectorDialogue.Broadcast(Dialogue, Intent);
}

void AShowDownGameStateBase::BroadcastCollectorLLMDecision(const FString& Dialogue, const FString& Intent, EShowDownBetAction Action, int32 TargetBet)
{
	if (HasAuthority())
	{
		MulticastCollectorLLMDecision(Dialogue, Intent, Action, TargetBet);
		return;
	}

	OnCollectorDialogue.Broadcast(Dialogue, Intent);
	OnCollectorLLMDecision.Broadcast(Dialogue, Intent, Action, TargetBet);
	OnCollectorLLMStatus.Broadcast(true, TEXT("Boss answered."));
}

void AShowDownGameStateBase::BroadcastCollectorLLMStatus(bool bSuccess, const FString& Message)
{
	if (HasAuthority())
	{
		MulticastCollectorLLMStatus(bSuccess, Message);
		return;
	}

	OnCollectorLLMStatus.Broadcast(bSuccess, Message);
}

void AShowDownGameStateBase::BroadcastChatMessage(const FString& SenderName, const FString& Message)
{
	if (HasAuthority())
	{
		MulticastChatMessage(SenderName, Message);
		return;
	}

	OnChatMessageReceived.Broadcast(SenderName, Message);
}

void AShowDownGameStateBase::MulticastCollectorLLMDecision_Implementation(const FString& Dialogue, const FString& Intent, EShowDownBetAction Action, int32 TargetBet)
{
	OnCollectorDialogue.Broadcast(Dialogue, Intent);
	OnCollectorLLMDecision.Broadcast(Dialogue, Intent, Action, TargetBet);
	OnCollectorLLMStatus.Broadcast(true, TEXT("Boss answered."));
}

void AShowDownGameStateBase::MulticastCollectorLLMStatus_Implementation(bool bSuccess, const FString& Message)
{
	OnCollectorLLMStatus.Broadcast(bSuccess, Message);
}

void AShowDownGameStateBase::MulticastChatMessage_Implementation(const FString& SenderName, const FString& Message)
{
	OnChatMessageReceived.Broadcast(SenderName, Message);
}
