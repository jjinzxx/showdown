// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShowDownGameModeBase.generated.h"

class ACard;
class APlayerPawn;

/**
 * 
 */
UCLASS()
class SHOWDOWN_API AShowDownGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void PlayerSelectedCard(ACard* SelectedCard);

	//스폰할 카드 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShowDown|Card")
	TSubclassOf<ACard> CardClass;
	// 플레이어에게 줄 카드 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	int32 HandCount = 5;
	//카드 사이 간격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float HandSpacing = 55.0f;
	//카드 위치
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
	//덱을 만들고 섞은 뒤에 플레이어에게 5장 스폰
	void DealPlayerInitialHand();
	// 1~7까지의 카드 가 두장씩 있는 14장의덱 생성
	void BuildDeck(TArray<int32>& OutDeck) const;
	// 덱을 무작위로 섞음
	void ShuffleDeck(TArray<int32>& Deck) const;
	
};
