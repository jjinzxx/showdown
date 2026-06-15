// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Actor.h"
#include "SDCameraBlendTester.generated.h"

class ACameraActor;
class APlayerController;

UCLASS(Blueprintable)
class SHOWDOWN_API ASDCameraBlendTester : public AActor
{
	GENERATED_BODY()

public:
	ASDCameraBlendTester();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Camera Blend Test")
	TObjectPtr<ACameraActor> DefaultCamera;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Camera Blend Test")
	TObjectPtr<ACameraActor> CloseCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Blend Test", meta = (ClampMin = "0.0"))
	float BlendTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Blend Test", meta = (ClampMin = "0.0"))
	float FreeViewBlendTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Blend Test")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunction = VTBlend_EaseInOut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Blend Test", meta = (ClampMin = "0.0"))
	float BlendExponent = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Blend Test")
	bool bSetDefaultCameraOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Blend Test")
	bool bRestoreGameInputOnFreeView = true;

private:
	void BindCameraInput();
	void BlendToDefaultCamera();
	void BlendToCloseCamera();
	void BlendToFreeView();
	void BlendToCamera(ACameraActor* CameraActor, const TCHAR* CameraLabel) const;
	void BlendToViewTarget(AActor* ViewTarget, const TCHAR* ViewTargetLabel, float InBlendTime) const;
	APlayerController* GetLocalPlayerController() const;
};
