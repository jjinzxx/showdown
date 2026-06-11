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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShowDown|Card")
	TSubclassOf<ACard> CardClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	int32 PlayerHandCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float PlayerHandSpacing = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float PlayerHandForwardOffset = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float PlayerHandFanAngle = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card")
	float PlayerHandFanDepth = 25.0f;

protected:
	virtual void BeginPlay() override;

private:
	void DealPlayerInitialHand();
	void BuildDeck(TArray<int32>& OutDeck) const;
	void ShuffleDeck(TArray<int32>& Deck) const;
	
};
