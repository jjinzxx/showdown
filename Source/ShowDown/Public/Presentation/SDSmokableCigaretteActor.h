// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "Interaction/SDInteractable.h"
#include "SDSmokableCigaretteActor.generated.h"

class UParticleSystemComponent;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSDSmokableCigaretteEvent);

UCLASS(Blueprintable)
class SHOWDOWN_API ASDSmokableCigaretteActor : public AActor, public ISDInteractable
{
	GENERATED_BODY()

public:
	ASDSmokableCigaretteActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Smokable Cigarette")
	void Smoke();

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(BlueprintAssignable, Category = "Smokable Cigarette")
	FSDSmokableCigaretteEvent OnSmokeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Smokable Cigarette")
	FSDSmokableCigaretteEvent OnSmokeFinished;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CigaretteMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> EmberLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> SmokeParticle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette")
	FVector LocalSmokeOffset = FVector(0.0f, 0.0f, 10.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette")
	FRotator LocalSmokeRotationOffset = FRotator(-12.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "First Person Smoke")
	bool bUseFirstPersonSmokePose = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "First Person Smoke")
	FVector FirstPersonCameraOffset = FVector(32.0f, 10.0f, -12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "First Person Smoke")
	FRotator FirstPersonRotationOffset = FRotator(0.0f, 0.0f, -18.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "First Person Smoke")
	bool bDisableCollisionWhileSmoking = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette", meta = (ClampMin = "0.01"))
	float LiftTime = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette", meta = (ClampMin = "0.0"))
	float SmokeHoldTime = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette", meta = (ClampMin = "0.01"))
	float ReturnTime = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette", meta = (ClampMin = "0.0"))
	float EmberIdleIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette", meta = (ClampMin = "0.0"))
	float EmberSmokeIntensity = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette", meta = (ClampMin = "0.0"))
	float EmberFlickerAmount = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette", meta = (ClampMin = "0.0"))
	float EmberFlickerSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokable Cigarette")
	TObjectPtr<USoundBase> SmokeSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	bool bEnableMouseClick = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	bool bEnablePlayerClickEventsOnBeginPlay = true;

private:
	enum class ECigaretteAnimState : uint8
	{
		Idle,
		Lifting,
		Holding,
		Returning
	};

	UFUNCTION()
	void HandleCigaretteClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

	void SetCigarettePoseAlpha(float Alpha);
	FTransform GetFirstPersonSmokeTransform() const;
	void SetActorTransformAlpha(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha);
	void UpdateEmber(float DeltaSeconds);
	void FinishSmoke();

	FTransform RestActorTransform;
	FTransform SmokeStartActorTransform;
	FTransform ReturnStartActorTransform;
	FVector RestRelativeLocation = FVector::ZeroVector;
	FVector SmokeRelativeLocation = FVector::ZeroVector;
	FRotator RestRelativeRotation = FRotator::ZeroRotator;
	FRotator SmokeRelativeRotation = FRotator::ZeroRotator;
	ECollisionEnabled::Type OriginalCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	ECigaretteAnimState AnimState = ECigaretteAnimState::Idle;
	float StateElapsedTime = 0.0f;
	float SmokeElapsedTime = 0.0f;
};
