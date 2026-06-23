#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDMultiplayerTable.generated.h"

class UStaticMeshComponent;

// C++ fallback board used when the multiplayer map has no authored card table.
UCLASS()
class SHOWDOWN_API ASDMultiplayerTable : public AActor
{
	GENERATED_BODY()

public:
	ASDMultiplayerTable();

private:
	UPROPERTY(VisibleAnywhere, Category = "ShowDown|Multiplayer")
	TObjectPtr<UStaticMeshComponent> TableSurface;
};
