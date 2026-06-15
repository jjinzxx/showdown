// Fill out your copyright notice in the Description page of Project Settings.

#include "Presentation/SDCameraBlendTester.h"

#include "Camera/CameraActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

ASDCameraBlendTester::ASDCameraBlendTester()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoReceiveInput = EAutoReceiveInput::Player0;
}

void ASDCameraBlendTester::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		EnableInput(PlayerController);
		BindCameraInput();

		if (bSetDefaultCameraOnBeginPlay && DefaultCamera)
		{
			PlayerController->SetViewTarget(DefaultCamera);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SDCameraBlendTester: PlayerController not found."));
	}
}

void ASDCameraBlendTester::BindCameraInput()
{
	if (!InputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("SDCameraBlendTester: InputComponent not found."));
		return;
	}

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASDCameraBlendTester::BlendToDefaultCamera);
	InputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &ASDCameraBlendTester::BlendToDefaultCamera);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ASDCameraBlendTester::BlendToCloseCamera);
	InputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &ASDCameraBlendTester::BlendToCloseCamera);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ASDCameraBlendTester::BlendToFreeView);
	InputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &ASDCameraBlendTester::BlendToFreeView);
}

void ASDCameraBlendTester::BlendToDefaultCamera()
{
	BlendToCamera(DefaultCamera, TEXT("DefaultCamera"));
}

void ASDCameraBlendTester::BlendToCloseCamera()
{
	BlendToCamera(CloseCamera, TEXT("CloseCamera"));
}

void ASDCameraBlendTester::BlendToFreeView()
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("SDCameraBlendTester: FreeView blend failed - PlayerController not found."));
		return;
	}

	APawn* ControlledPawn = PlayerController->GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("SDCameraBlendTester: FreeView blend failed - controlled Pawn not found."));
		return;
	}

	if (bRestoreGameInputOnFreeView)
	{
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}

	PlayerController->SetViewTargetWithBlend(
		ControlledPawn,
		FreeViewBlendTime,
		BlendFunction,
		BlendExponent,
		false
	);
}

void ASDCameraBlendTester::BlendToCamera(ACameraActor* CameraActor, const TCHAR* CameraLabel) const
{
	BlendToViewTarget(CameraActor, CameraLabel, BlendTime);
}

void ASDCameraBlendTester::BlendToViewTarget(AActor* ViewTarget, const TCHAR* ViewTargetLabel, float InBlendTime) const
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("SDCameraBlendTester: %s blend failed - PlayerController not found."), ViewTargetLabel);
		return;
	}

	if (!ViewTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("SDCameraBlendTester: %s is not assigned."), ViewTargetLabel);
		return;
	}

	PlayerController->SetViewTargetWithBlend(
		ViewTarget,
		InBlendTime,
		BlendFunction,
		BlendExponent,
		false
	);
}

APlayerController* ASDCameraBlendTester::GetLocalPlayerController() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}
