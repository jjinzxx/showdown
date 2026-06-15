// Fill out your copyright notice in the Description page of Project Settings.


#include "Collector.h"


// Sets default values
ACollector::ACollector()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// 루트 메시 컴포넌트 생성
	rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(rootComp);
	// 손패 카드 슬롯
	c_HandCard = CreateDefaultSubobject<USceneComponent>(TEXT("CollectorHandCardSlot"));
	c_HandCard->SetupAttachment(rootComp);
	//이마 카드 슬롯
	c_HeadCard = CreateDefaultSubobject<USceneComponent>(TEXT("CollectorHeadCardSlot"));
	c_HeadCard->SetupAttachment(rootComp);
}

// Called when the game starts or when spawned
void ACollector::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACollector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}




