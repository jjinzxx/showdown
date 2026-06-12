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

	UFUNCTION(BlueprintCallable, Category = "ShowDown|CollectorAI")
	int32 ChooseCardToGive(const TArray<int32>& HandRanks) const;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|CollectorAI")
	ACard* ChooseCardActorToGive(const TArray<ACard*>& HandCards) const;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|CollectorAI")
	EShowDownBetAction ChooseBetAction(float EstimatedWinChance, int32 CurrentBet) const;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|CollectorAI")
	EShowDownBetAction ChooseBetActionByGivenCard(int32 GivenCardRank, int32 CurrentBet) const;
};

