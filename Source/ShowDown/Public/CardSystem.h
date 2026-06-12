// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CardSystem.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOWDOWN_API UCardSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCardSystem();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void ResetDeck();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void ShuffleDeck();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	bool DealCards(int32 Count, TArray<int32>& OutCards);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void DiscardCard(int32 Rank);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void DiscardCards(const TArray<int32>& Ranks);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Card")
	int32 GetRemainingCardCount() const;

private:
	UPROPERTY()
	TArray<int32> Deck;

	UPROPERTY()
	TArray<int32> DiscardPile;

	void BuildDeck();
};
