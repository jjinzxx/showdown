#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SDPlayerState.generated.h"

class ACard;

UCLASS()
class SHOWDOWN_API ASDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	//플레이어가 들고 있는 손패 카드 목록
	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Card")
	TArray<ACard*> HandCards;

	//플레이어 이마에 올라간 카드
	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Card")
	ACard* ForeheadCard = nullptr;

	//플레이어 현재 목숨
	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|State")
	int32 Lives = 3;

	//플레이어 현재 베팅 총알 수
	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Betting")
	int32 CurrentBet = 0;

	//손패에 카드 추가
	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void AddHandCard(ACard* Card);

	//손패에서 카드 제거
	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void RemoveHandCard(ACard* Card);

	//손패 전체 비우기
	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void ClearHand();
};
