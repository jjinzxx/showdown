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
	//게임 단계가 바뀔 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownPhaseChangedSignature OnPhaseChanged;

	//베팅 값이 바뀔 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownGameStateBetChangedSignature OnBetChanged;

	//양쪽 카드가 공개될 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownCardsRevealedSignature OnCardsRevealed;

	//라운드 승패가 결정될 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownRoundResolvedSignature OnRoundResolved;

	//룰렛이 시작될 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownRouletteStartedSignature OnRouletteStarted;

	//룰렛 결과가 나왔을 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownRouletteResultSignature OnRouletteResult;

	//목숨이 변했을 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownLifeChangedSignature OnLifeChanged;

	//스테이지가 바뀔 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownStageChangedSignature OnStageChanged;

	//게임이 끝났을 때 UI/연출에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Events")
	FShowDownGameOverSignature OnGameOver;

	//현재 게임 진행 단계
	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|State")
	EShowDownPhase CurrentPhase = EShowDownPhase::None;

	//현재 스테이지 번호
	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|State")
	int32 CurrentStage = 1;

	//현재 라운드 번호
	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|State")
	int32 CurrentRound = 1;

	//현재 게임 진행 단계 변경
	UFUNCTION(BlueprintCallable, Category = "ShowDown|State")
	void SetPhase(EShowDownPhase NewPhase);
};
