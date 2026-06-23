#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/SDInteractable.h"
#include "ShowDownTypes.h"
#include "Card.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class FLifetimeProperty;

UCLASS()
class SHOWDOWN_API ACard : public AActor, public ISDInteractable
{
	GENERATED_BODY()

public:
	ACard();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* VisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CardMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* CardText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionBounds;

	UPROPERTY(ReplicatedUsing = OnRep_CardVisual, EditAnywhere, BlueprintReadWrite, Category = "Card", meta = (ClampMin = "1", ClampMax = "7"))
	int32 Rank = 1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Card")
	bool bSelectable = true;

	UPROPERTY(ReplicatedUsing = OnRep_CardVisual, EditAnywhere, BlueprintReadWrite, Category = "Card")
	bool bFaceUp = false;

	UPROPERTY(ReplicatedUsing = OnRep_CardVisual, BlueprintReadOnly, Category = "Card")
	EShowDownPlayerSlot HiddenFromSlot = EShowDownPlayerSlot::None;

	UPROPERTY(ReplicatedUsing = OnRep_CardVisual, BlueprintReadOnly, Category = "Card")
	EShowDownPlayerSlot HandOwnerSlot = EShowDownPlayerSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Select")
	FVector SelectedOffset = FVector(0.0f, 0.0f, 12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Hover")
	FVector HoverOffset = FVector(0.0f, 0.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Select")
	float MoveSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Interaction")
	FVector InteractionBoundsExtent = FVector(45.0f, 65.0f, 8.0f);

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetCard(int32 NewRank);

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetFaceUp(bool bNewFaceUp);

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetHiddenFromSlot(EShowDownPlayerSlot NewHiddenFromSlot);

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetHandOwnerSlot(EShowDownPlayerSlot NewHandOwnerSlot);

	UFUNCTION(BlueprintCallable, Category = "Card")
	void RefreshVisual();

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SelectCard(bool bNewSelected);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Card")
	bool IsSelected() const;

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetHovered(bool bNewHovered);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Card")
	bool IsHovered() const;

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetSelectable(bool bNewSelectable);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Card")
	bool IsCardSelectable() const;

	UFUNCTION(BlueprintCallable, Category = "Card")
	void MoveToSlot(USceneComponent* Slot, bool bNewFaceUp);

	UFUNCTION(BlueprintCallable, Category = "Card")
	void MoveToHandTransform(const FTransform& NewTransform);

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CardVisual();

public:
	virtual void Tick(float DeltaTime) override;

private:
	void ConfigureInteractionComponents();
	void UpdateTargetTransform();
	void EnableMotionTick();

	UPROPERTY(VisibleAnywhere, Category = "Card")
	bool bSelected = false;

	UPROPERTY(VisibleAnywhere, Category = "Card")
	bool bHovered = false;

	FVector DefaultLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FVector CurrentVisualWorldOffset = FVector::ZeroVector;
	FVector TargetVisualWorldOffset = FVector::ZeroVector;
	FRotator DefaultRotation = FRotator::ZeroRotator;
	FRotator TargetRotation = FRotator::ZeroRotator;
	int32 CachedVisualRank = INDEX_NONE;
	bool bCachedVisualVisible = false;
	bool bHasCachedVisual = false;
};
