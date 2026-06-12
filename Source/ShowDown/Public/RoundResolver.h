#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShowDownTypes.h"
#include "RoundResolver.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOWDOWN_API URoundResolver : public UActorComponent
{
	GENERATED_BODY()

public:
	URoundResolver();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Round")
	EShowDownRoundResult ResolveRevealedCards(int32 PlayerCard, int32 CollectorCard) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Round")
	int32 GetFoldLoadCount(int32 FoldedForeheadCard, int32 CurrentBet) const;
};
