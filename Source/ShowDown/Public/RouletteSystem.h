#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RouletteSystem.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOWDOWN_API URouletteSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	URouletteSystem();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Roulette")
	bool RollRoulette(int32 BulletCount) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Roulette")
	float GetHitChance(int32 BulletCount) const;
};
