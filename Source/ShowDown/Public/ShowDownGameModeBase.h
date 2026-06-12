// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShowDownGameModeBase.generated.h"

class ACard;
class APlayerPawn;
class UCardSystem;
class ACollector;

//각 플레이어(콜렉터, 플레이어, 멀티플레이어) 에 대한 값(손패, 이마의 카드, 목숨, 베팅값) 구조체로 저장
USTRUCT(BlueprintType)
struct FShowDownParticipantState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<ACard*> HandCards;

	UPROPERTY()
	ACard* ForeheadCard = nullptr;

	UPROPERTY()
	int32 Lives = 3;

	UPROPERTY()
	int32 CurrentBet = 0;
};

UCLASS()
class SHOWDOWN_API AShowDownGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShowDownGameModeBase();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void PlayerSelectedCard(ACard* SelectedCard);

	// 덱 관리 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShowDown|Deck")
	UCardSystem* CardSystem;

	// 스폰할 카드 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShowDown|Card")
	TSubclassOf<ACard> CardClass;

	// 플레이어에게 줄 카드 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	int32 HandCount = 5;

	// 카드 사이 간격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float HandSpacing = 55.0f;

	// 카드 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float HandForwardOffset = 180.0f;

	// 카드 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float HandFanAngle = 40.0f;

	// 카드가 양끝으로 빠지는 정도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float HandFanDepth = 25.0f;

protected:
	virtual void BeginPlay() override;

private:
	// 덱을 만들고 섞은 뒤에 플레이어에게 5장 스폰
	void DealInitialHand();
	
	//플레이어 스탯
	UPROPERTY()
	FShowDownParticipantState PlayerState;
	//콜렉터 스탯
	UPROPERTY()
	FShowDownParticipantState CollectorState;
	
	//콜렉터 추적
	UPROPERTY()
	ACollector* Collector = nullptr;
	void FindCollector();
	
};
