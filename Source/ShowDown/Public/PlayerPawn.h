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
	APlayerPawn();
	
	//루트 컴포넌트
	UPROPERTY(VisibleAnywhere, Category= Components)
	USceneComponent* rootComp;
	
	//카메라 컴포넌트 선언 
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* cameraComp;
	
	//손패 카드 슬롯 위치 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = CardSlot)
	class USceneComponent* PlayerHandCard;
	
	//상대가 나에게 주는 카드 위치 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = CardSlot)
	class USceneComponent* PlayerHeadCard;
	
	//IMC 선택 필드 선언
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputMappingContext* imc_SD;
	
	//카드 선택 액션
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* ia_Select;
	
	//카메라 무브
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* ia_LookUp;//상하
	UPROPERTY(EditDefaultsOnly, Category = Input)
	class UInputAction* ia_Turn;//좌우
	
	//카메라 회전 감도
	UPROPERTY(EditAnywhere, Category = Camera)
	float LookSensitivity = 0.08f;
	
	//카메라 회전 각도 제한
	//상하
	UPROPERTY(EditAnywhere, Category = Camera)
	float MinPitch = -35.0f;
	UPROPERTY(EditAnywhere, Category = Camera)
	float MaxPitch = 35.0f;
	//좌우
	UPROPERTY(EditAnywhere, Category = Camera)
	float MinYaw = -45.0f;
	UPROPERTY(EditAnywhere, Category = Camera)
	float MaxYaw = 45.0f;
	
	/*
	//
	UPROPERTY(VisibleAnywhere, Category="Table")
	USceneComponent* TableFocusPoint;
	*/
	
	//모드베이스 참조
	UPROPERTY()
	class AShowDownGameModeBase* ModeBase;
	
	//카드참조
	UPROPERTY()
	class ACard* HandCard;
	UPROPERTY()
	class ACard* CurrentSelectedCard;
	

	//카드 감지
	void InputSelect(const FInputActionValue& InputValue);
	//카메라 회전
	void LookUp(const FInputActionValue& InputValue);
	void Turn(const FInputActionValue& InputValue);
	//마우스 아래에 있는 카드 탐지
	void TraceCardUnderCursor();
	//선택된 카드를 다시 누르면 제출
	void SelectCard(ACard* SelectedCard);
	//선택된 카드를 게임 모드 베이스를 통해 전달
	void SubmitSelectedCard(ACard* SelectedCard);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	//IMC등록
	void AddInputMappingContext();
	void ApplyCameraInput(float YawInput, float PitchInput);

	bool bHasPreviousMousePosition = false;
	float PreviousMouseX = 0.0f;
	float PreviousMouseY = 0.0f;
	
	void HandleBettingHotkeys();
};
