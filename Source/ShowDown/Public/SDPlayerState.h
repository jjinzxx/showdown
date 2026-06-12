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
	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Card")
	TArray<ACard*> HandCards;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Card")
	ACard* ForeheadCard = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|State")
	int32 Lives = 3;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Betting")
	int32 CurrentBet = 0;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void AddHandCard(ACard* Card);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void RemoveHandCard(ACard* Card);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void ClearHand();
};
