#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDCardLayoutTypes.h"
#include "SDCardPlacementAnchor.generated.h"

class USceneComponent;

UENUM(BlueprintType)
enum class ESDCardPlacementRole : uint8
{
	PlayerHand UMETA(DisplayName = "Player Hand"),
	PlayerForehead UMETA(DisplayName = "Player Forehead"),
	OpponentHand UMETA(DisplayName = "Opponent Hand"),
	OpponentForehead UMETA(DisplayName = "Opponent Forehead")
};

UCLASS(Blueprintable)
class SHOWDOWN_API ASDCardPlacementAnchor : public AActor
{
	GENERATED_BODY()

public:
	ASDCardPlacementAnchor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShowDown|Placement")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShowDown|Placement")
	TObjectPtr<USceneComponent> CardSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Placement", meta = (DisplayName = "Placement Role"))
	ESDCardPlacementRole PlacementRole = ESDCardPlacementRole::PlayerHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout")
	ESDHandLayoutStyle HandLayoutStyle = ESDHandLayoutStyle::AutoFan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "0.0"))
	float FanWidth = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "0.0"))
	float FanDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "1.0"))
	float GripToCenterDistance = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "0.0"))
	float AnglePerGap = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "0.0"))
	float MaxFanAngle = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout")
	float LayerStep = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "-85.0", ClampMax = "85.0"))
	float FaceTiltAngle = -15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "-45.0", ClampMax = "45.0"))
	float EdgeCurlAngle = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CameraFacingStrength = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Hand Layout", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float MaxCameraFacingAngle = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card Motion")
	FVector SelectedOffset = FVector(0.0f, 0.0f, 12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card Motion")
	FVector HoverOffset = FVector(0.0f, 0.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Card Motion", meta = (ClampMin = "0.0"))
	float MoveSpeed = 10.0f;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Placement")
	USceneComponent* GetSlotComponent() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Placement")
	bool IsHandAnchor() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Placement")
	bool IsForeheadAnchor() const;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
};

UCLASS(Blueprintable)
class SHOWDOWN_API ASDPlayerHandAnchor : public ASDCardPlacementAnchor
{
	GENERATED_BODY()

public:
	ASDPlayerHandAnchor();
};

UCLASS(Blueprintable)
class SHOWDOWN_API ASDPlayerForeheadAnchor : public ASDCardPlacementAnchor
{
	GENERATED_BODY()

public:
	ASDPlayerForeheadAnchor();
};

UCLASS(Blueprintable)
class SHOWDOWN_API ASDOpponentHandAnchor : public ASDCardPlacementAnchor
{
	GENERATED_BODY()

public:
	ASDOpponentHandAnchor();
};

UCLASS(Blueprintable)
class SHOWDOWN_API ASDOpponentForeheadAnchor : public ASDCardPlacementAnchor
{
	GENERATED_BODY()

public:
	ASDOpponentForeheadAnchor();
};
