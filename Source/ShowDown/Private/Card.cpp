// Fill out your copyright notice in the Description page of Project Settings.


#include "Card.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

// Sets default values
ACard::ACard()
{
	PrimaryActorTick.bCanEverTick = true;

	//루트컴프
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	//메시 컴프
	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	CardMesh->SetupAttachment(RootComp);
	CardMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CardMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CardMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	CardMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	//카드 숫자 텍스트
	CardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CardText"));
	CardText->SetupAttachment(RootComp);
	CardText->SetHorizontalAlignment(EHTA_Center);
	CardText->SetVerticalAlignment(EVRTA_TextCenter);
	CardText->SetTextRenderColor(FColor::Black);
	CardText->SetWorldSize(30.0f);
	CardText->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
}

// Called when the game starts or when spawned
void ACard::BeginPlay()
{
	Super::BeginPlay();

	DefaultLocation = GetActorLocation();
	TargetLocation = DefaultLocation;
	RefreshVisual();
}

// Called every frame
void ACard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector CurrentLocation = GetActorLocation();
	const FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, MoveSpeed);
	SetActorLocation(NewLocation);
}

void ACard::SetCard(ECardSuit NewSuit, int32 NewRank)
{
	Suit = NewSuit;
	Rank = FMath::Clamp(NewRank, 1, 7);
	RefreshVisual();
}

void ACard::SetFaceUp(bool bNewFaceUp)
{
	bFaceUp = bNewFaceUp;
	RefreshVisual();
}

void ACard::RefreshVisual()
{
	if (!CardText)
	{
		return;
	}

	CardText->SetVisibility(bFaceUp);
	CardText->SetText(FText::AsNumber(Rank));
}

void ACard::SelectCard(bool bNewSelected)
{
	bSelected = bNewSelected;
	TargetLocation = bSelected ? DefaultLocation + SelectedOffset : DefaultLocation;
}

bool ACard::IsSelected() const
{
	return bSelected;
}
