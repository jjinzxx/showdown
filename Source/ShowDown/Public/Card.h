// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Card.generated.h"

//카드 무늬
UENUM(BlueprintType)
enum class ECardSuit : uint8
{
	Spade UMETA(DisplayName = "Spade"),
	Heart UMETA(DisplayName = "Heart")
};

UCLASS()
class SHOWDOWN_API ACard : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACard();

	//루트 컴프
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* RootComp;
	//메쉬 컴프
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* CardMesh;
	//카드 숫자 표시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UTextRenderComponent* CardText;
	//카드 무늬
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	ECardSuit Suit = ECardSuit::Spade;
	//카드 숫자 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card", meta = (ClampMin = "1", ClampMax = "7"))
	int32 Rank = 1;
	//앞면 뒷면 체크 false = 뒷면
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	bool bFaceUp = false;
	//선택 연출(카드가 선택 되었을 때 원래 위치에서 이동되는 값) 위 방향으로 12만큼 이동
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Select")
	FVector SelectedOffset = FVector(0.0f, 0.0f, 12.0f);
	//카드 선택 위치까지 이동하는 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Select")
	float MoveSpeed = 10.0f;
	//카드의 무늬와 숫자를 설정하는 함수
	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetCard(ECardSuit NewSuit, int32 NewRank);
	//앞뒷면 설정
	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetFaceUp(bool bNewFaceUp);
	//카드 외형 갱신
	UFUNCTION(BlueprintCallable, Category = "Card")
	void RefreshVisual();
	//선택상태 변경
	UFUNCTION(BlueprintCallable, Category = "Card")
	void SelectCard(bool bNewSelected);
	// 석
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Card")
	bool IsSelected() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Card")
	bool bSelected = false;

	FVector DefaultLocation;
	FVector TargetLocation;
};
