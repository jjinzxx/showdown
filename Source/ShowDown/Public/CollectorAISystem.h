#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShowDownTypes.h"
#include "CollectorAISystem.generated.h"

class ACard;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOWDOWN_API UCollectorAISystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UCollectorAISystem();

	//숫자 손패에서 콜렉터가 줄 카드 숫자 선택
	UFUNCTION(BlueprintCallable, Category = "ShowDown|CollectorAI")
	int32 ChooseCardToGive(const TArray<int32>& HandRanks) const;

	//실제 카드 액터 손패에서 콜렉터가 줄 카드 선택
	UFUNCTION(BlueprintCallable, Category = "ShowDown|CollectorAI")
	ACard* ChooseCardActorToGive(const TArray<ACard*>& HandCards) const;

	//추정 승률 기준으로 콜렉터 베팅 행동 선택
	UFUNCTION(BlueprintCallable, Category = "ShowDown|CollectorAI")
	EShowDownBetAction ChooseBetAction(float EstimatedWinChance, int32 CurrentBet) const;

	//콜렉터가 플레이어에게 준 카드 숫자를 기준으로 베팅 행동 선택
	UFUNCTION(BlueprintCallable, Category = "ShowDown|CollectorAI")
	EShowDownBetAction ChooseBetActionByGivenCard(int32 GivenCardRank, int32 CurrentBet) const;
};
