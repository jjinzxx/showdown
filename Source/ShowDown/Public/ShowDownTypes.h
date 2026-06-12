#pragma once

#include "CoreMinimal.h"
#include "ShowDownTypes.generated.h"

UENUM(BlueprintType)
enum class EShowDownPhase : uint8
{
	None,
	SelectCard,
	Betting,
	Reveal,
	Roulette,
	RoundEnd,
	GameOver
};

UENUM(BlueprintType)
enum class EShowDownSide : uint8
{
	Player,
	Collector
};

UENUM(BlueprintType)
enum class EShowDownRoundResult : uint8
{
	PlayerWin,
	CollectorWin,
	Draw
};

UENUM(BlueprintType)
enum class EShowDownBetAction : uint8
{
	Check,
	Call,
	Raise,
	Fold
};
