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

UENUM(BlueprintType)
enum class EShowDownMatchMode : uint8
{
	SinglePlayer,
	Multiplayer
};

UENUM(BlueprintType)
enum class EShowDownPlayerSlot : uint8
{
	None,
	Player1,
	Player2,
	Player3,
	Player4
};

USTRUCT(BlueprintType)
struct FShowDownNetworkPlayerSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Multiplayer")
	EShowDownPlayerSlot Slot = EShowDownPlayerSlot::None;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Multiplayer")
	FString PlayerId;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Multiplayer")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Multiplayer")
	bool bConnected = false;

	UPROPERTY(BlueprintReadOnly, Category = "ShowDown|Multiplayer")
	bool bReady = false;
};
