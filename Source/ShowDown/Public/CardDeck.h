// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CardDeck.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOWDOWN_API UCardDeck : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCardDeck();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	
	void ShuffleDeck();
	void ResetDeck();
	bool DealCards(int32 Count, TArray<int32>& OutCards);
	
private:
	//덱 배열 생성
	UPROPERTY()
	TArray<int32> Deck;
	//공개된 카드
	UPROPERTY()
	TArray<int32> DiscardPile;
	// 1~7까지의 카드 가 두장씩 있는 14장의덱 생성
	void BuildDeck();
};
