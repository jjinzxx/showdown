#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ShowDownTypes.h"
#include "ShowDownGameStateBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShowDownPhaseChangedSignature, EShowDownPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowDownGameStateBetChangedSignature, EShowDownSide, Side, int32, BulletCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowDownCardsRevealedSignature, int32, PlayerCard, int32, CollectorCard);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShowDownRoundResolvedSignature, EShowDownRoundResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowDownRouletteStartedSignature, EShowDownSide, Target, int32, BulletCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowDownRouletteResultSignature, EShowDownSide, Target, bool, bHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowDownLifeChangedSignature, EShowDownSide, Target, int32, Life);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShowDownStageChangedSignature, int32, Stage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShowDownGameOverSignature, EShowDownSide, Winner);

UCLASS()
class SHOWDOWN_API AShowDownGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownGameStateBetChangedSignature OnBetChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownCardsRevealedSignature OnCardsRevealed;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownRoundResolvedSignature OnRoundResolved;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownRouletteStartedSignature OnRouletteStarted;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownRouletteResultSignature OnRouletteResult;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownLifeChangedSignature OnLifeChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownStageChangedSignature OnStageChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownGameOverSignature OnGameOver;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|State")
	EShowDownPhase CurrentPhase = EShowDownPhase::None;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|State")
	int32 CurrentStage = 1;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|State")
	int32 CurrentRound = 1;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|State")
	void SetPhase(EShowDownPhase NewPhase);
};
