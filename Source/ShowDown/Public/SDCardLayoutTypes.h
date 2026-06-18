#pragma once

#include "CoreMinimal.h"
#include "SDCardLayoutTypes.generated.h"

UENUM(BlueprintType)
enum class ESDHandLayoutStyle : uint8
{
	AutoFan UMETA(DisplayName = "Auto Fan"),
	FixedFan UMETA(DisplayName = "Fixed Fan")
};

USTRUCT(BlueprintType)
struct FSDCardHandLayoutSettings
{
	GENERATED_BODY()

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
};
