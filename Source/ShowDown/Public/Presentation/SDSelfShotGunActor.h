#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/SDInteractable.h"
#include "Presentation/SDArtToneController.h"
#include "SDSelfShotGunActor.generated.h"

class UBoxComponent;
class UPointLightComponent;
class USceneComponent;
class USoundBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSDSelfShotGunEvent);

UENUM(BlueprintType)
enum class ESDHitSequenceEaseMode : uint8
{
	Linear UMETA(DisplayName = "Linear"),
	EaseIn UMETA(DisplayName = "Ease In (Slow To Fast)"),
	EaseOut UMETA(DisplayName = "Ease Out (Fast To Slow)"),
	EaseInOut UMETA(DisplayName = "Ease In Out")
};

UCLASS(Blueprintable)
class SHOWDOWN_API ASDSelfShotGunActor : public AActor, public ISDInteractable
{
	GENERATED_BODY()

public:
	ASDSelfShotGunActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Self Shot Gun")
	void UseGun();

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(BlueprintAssignable, Category = "Self Shot Gun")
	FSDSelfShotGunEvent OnGunRaised;

	UPROPERTY(BlueprintAssignable, Category = "Self Shot Gun")
	FSDSelfShotGunEvent OnGunFired;

	UPROPERTY(BlueprintAssignable, Category = "Self Shot Gun")
	FSDSelfShotGunEvent OnGunSequenceFinished;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GunMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ChamberPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ChamberMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> TriggerPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TriggerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RatchetMechanismMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> MuzzleFlashLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|First Person")
	bool bUseFirstPersonPose = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|First Person")
	FVector FirstPersonCameraOffset = FVector(34.0f, 26.0f, -18.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|First Person")
	FRotator FirstPersonRotationOffset = FRotator(-22.0f, 0.0f, -58.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Timing", meta = (ClampMin = "0.01"))
	float RaiseTime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Timing", meta = (ClampMin = "0.0"))
	float AimHoldTime = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Timing", meta = (ClampMin = "0.0"))
	float ShotHoldTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Timing", meta = (ClampMin = "0.01"))
	float ReturnTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Effects")
	TObjectPtr<USoundBase> GunshotSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Effects")
	bool bPlayGunshotSound2D = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Muzzle Flash", meta = (ClampMin = "0.0"))
	float MuzzleFlashIntensity = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Muzzle Flash", meta = (ClampMin = "0.01"))
	float MuzzleFlashDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Muzzle Flash", meta = (ClampMin = "0.0"))
	float MuzzleFlashAttenuationRadius = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Muzzle Flash")
	FLinearColor MuzzleFlashColor = FLinearColor(1.0f, 0.52f, 0.16f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence")
	bool bEnableHitSequence = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence")
	TObjectPtr<ASDArtToneController> HitSequenceArtToneController;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Stage 1")
	FSDArtToneSettings InitialHitEffectSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Stage 1", meta = (ClampMin = "0.0"))
	float InitialHitEffectDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Stage 1")
	FRotator InitialHitShakeRotationAmplitude = FRotator(2.4f, 1.2f, 1.8f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Stage 1")
	FVector InitialHitShakeLocationAmplitude = FVector(0.8f, 1.6f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Stage 1", meta = (ClampMin = "0.01"))
	float InitialHitShakeStepInterval = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Blackout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitBlackoutAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Blackout", meta = (ClampMin = "0.0"))
	float HitBlackoutDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery")
	FSDArtToneSettings RecoveryHitEffectSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery", meta = (ClampMin = "0.0"))
	float RecoveryHitEffectHoldTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery", meta = (ClampMin = "0.01"))
	float RecoveryHitEffectBlendOutTime = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery")
	ESDHitSequenceEaseMode RecoveryHitEffectEaseMode = ESDHitSequenceEaseMode::EaseIn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery", meta = (ClampMin = "1.0"))
	float RecoveryHitEffectEaseExponent = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery")
	FRotator RecoveryHitShakeRotationAmplitude = FRotator(1.2f, 1.8f, 2.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery")
	FVector RecoveryHitShakeLocationAmplitude = FVector(0.35f, 1.3f, 0.75f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery", meta = (ClampMin = "0.0"))
	float RecoveryHitShakeHoldTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery", meta = (ClampMin = "0.01"))
	float RecoveryHitShakeBlendOutTime = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun|Hit Sequence|Recovery", meta = (ClampMin = "0.01"))
	float RecoveryHitShakeStepInterval = 0.075f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Self Shot Gun")
	bool bDisableCollisionWhileUsing = true;

private:
	enum class EGunAnimState : uint8
	{
		Idle,
		Raising,
		Aiming,
		Fired,
		Returning
	};

	enum class EHitSequenceState : uint8
	{
		Idle,
		InitialHit,
		Blackout,
		RecoveryHold,
		RecoveryBlendOut
	};

	void FireGun();
	void FinishSequence();
	void StartHitSequence();
	void UpdateHitSequence(float DeltaSeconds);
	void EnterHitSequenceBlackout();
	void EnterHitSequenceRecovery();
	void FinishHitSequence();
	void SetBlackoutInstant(float Alpha, bool bHoldWhenFinished);
	ASDArtToneController* ResolveHitSequenceArtToneController();
	FTransform GetFirstPersonGunTransform() const;
	void SetActorTransformAlpha(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha);

	FTransform RestActorTransform;
	FTransform RaiseStartTransform;
	FTransform ReturnStartTransform;
	ECollisionEnabled::Type OriginalCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	EGunAnimState AnimState = EGunAnimState::Idle;
	EHitSequenceState HitSequenceState = EHitSequenceState::Idle;
	float StateElapsedTime = 0.0f;
	float HitSequenceElapsedTime = 0.0f;
	float MuzzleFlashElapsedTime = 0.0f;
	FSDArtToneSettings HitSequenceBaseSettings;
};
