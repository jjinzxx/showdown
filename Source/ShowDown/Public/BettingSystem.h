#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShowDownTypes.h"
#include "BettingSystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowDownBetChangedSignature, EShowDownSide, Side, int32, BulletCount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOWDOWN_API UBettingSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UBettingSystem();

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Betting")
	FShowDownBetChangedSignature OnBetChanged;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	void ResetBetting(int32 MinimumBet = 0);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	void Check(EShowDownSide Side);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	void Call(EShowDownSide Side);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	bool RaiseTo(EShowDownSide Side, int32 BulletCount);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	void Fold(EShowDownSide Side);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Betting")
	int32 GetCurrentBet() const;

private:
	UPROPERTY()
	int32 CurrentBet = 0;
};
