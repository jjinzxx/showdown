#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShowDownTypes.h"
#include "BossAISystem.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOWDOWN_API UBossAISystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossAISystem();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|BossAI")
	int32 ChooseCardToGive(const TArray<int32>& HandRanks) const;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|BossAI")
	EShowDownBetAction ChooseBetAction(float EstimatedWinChance, int32 CurrentBet) const;
};
