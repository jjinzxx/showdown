// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Collector.generated.h"

UCLASS()
class SHOWDOWN_API ACollector : public AActor
{
	GENERATED_BODY()

public:
	ACollector();
	
	//루트 컴포넌트
	UPROPERTY(VisibleAnywhere, Category= Components)
	USceneComponent* rootComp;

	//손패 카드 슬롯 위치 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = CardSlot)
	class USceneComponent* c_HandCard;
	
	//상대가 나에게 주는 카드 위치 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = CardSlot)
	class USceneComponent* c_HeadCard;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
};
