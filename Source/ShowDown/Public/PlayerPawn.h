// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"


#include "PlayerPawn.generated.h"



UCLASS()
class SHOWDOWN_API APlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APlayerPawn();
	
	//루트 컴포넌트
	UPROPERTY(VisibleAnywhere, Category= Components)
	USceneComponent* rootComp;
	
	//카메라 컴포넌트 선언
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* cameraComp;
	
	//손패 카드 슬롯 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = CardSlot)
	class USceneComponent* PlayerHandCard;
	
	//상대가 나에게 주는 카드 슬롯 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = CardSlot)
	class USceneComponent* PlayerHeadCard;
	
	//IMC 선택 필드 선언
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputMappingContext* imc_SD;
	
	//선택 액션
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* ia_Select;
	
	//카메라 무브
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* ia_LookUp;
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* ia_Turn;
	
	//카메라 회전 감도
	UPROPERTY(EditAnywhere, Category = Camera)
	float LookSensitivity = 0.08f;
	
	UPROPERTY(EditAnywhere, Category = Camera)
	float MinPitch = -35.0f;

	UPROPERTY(EditAnywhere, Category = Camera)
	float MaxPitch = 35.0f;

	UPROPERTY(EditAnywhere, Category = Camera)
	float MinYaw = -45.0f;

	UPROPERTY(EditAnywhere, Category = Camera)
	float MaxYaw = 45.0f;
	
	/*
	//
	UPROPERTY(VisibleAnywhere, Category="Table")
	USceneComponent* TableFocusPoint;
	*/
	
	UPROPERTY()
	class AShowDownGameModeBase* ModeBase;
	
	UPROPERTY()
	class ACard* HandCard;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	void InputSelect(const FInputActionValue& InputValue);
	void LookUp(const FInputActionValue& InputValue);
	void Turn(const FInputActionValue& InputValue);
	void TraceCardUnderCursor();
	void SelectCard(ACard* SelectedCard);

private:
	void AddInputMappingContext();
	void ApplyCameraInput(float YawInput, float PitchInput);

	bool bHasPreviousMousePosition = false;
	float PreviousMouseX = 0.0f;
	float PreviousMouseY = 0.0f;
};
