#include "Presentation/SDSelfShotGunActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ShowDownPlayerController.h"
#include "Sound/SoundBase.h"

namespace
{
	float ApplyEase(float Alpha, ESDHitSequenceEaseMode EaseMode, float Exponent)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		const float ClampedExponent = FMath::Max(1.0f, Exponent);

		switch (EaseMode)
		{
		case ESDHitSequenceEaseMode::EaseIn:
			return FMath::InterpEaseIn(0.0f, 1.0f, ClampedAlpha, ClampedExponent);
		case ESDHitSequenceEaseMode::EaseOut:
			return FMath::InterpEaseOut(0.0f, 1.0f, ClampedAlpha, ClampedExponent);
		case ESDHitSequenceEaseMode::EaseInOut:
			return FMath::InterpEaseInOut(0.0f, 1.0f, ClampedAlpha, ClampedExponent);
		default:
			return ClampedAlpha;
		}
	}

	FSDArtToneSettings LerpArtToneSettings(const FSDArtToneSettings& From, const FSDArtToneSettings& To, float Alpha)
	{
		FSDArtToneSettings Result;
		Result.PixelCount = FMath::Lerp(From.PixelCount, To.PixelCount, Alpha);
		Result.ColorSteps = FMath::Lerp(From.ColorSteps, To.ColorSteps, Alpha);
		Result.HalftoneStrength = FMath::Lerp(From.HalftoneStrength, To.HalftoneStrength, Alpha);
		Result.HalftoneCount = FMath::Lerp(From.HalftoneCount, To.HalftoneCount, Alpha);
		Result.HalftoneRadius = FMath::Lerp(From.HalftoneRadius, To.HalftoneRadius, Alpha);
		Result.HalftoneSoftness = FMath::Lerp(From.HalftoneSoftness, To.HalftoneSoftness, Alpha);
		Result.HalftoneShape = FMath::Lerp(From.HalftoneShape, To.HalftoneShape, Alpha);
		Result.ExposureCompensation = FMath::Lerp(From.ExposureCompensation, To.ExposureCompensation, Alpha);
		Result.Saturation = FMath::Lerp(From.Saturation, To.Saturation, Alpha);
		Result.Contrast = FMath::Lerp(From.Contrast, To.Contrast, Alpha);
		Result.VignetteIntensity = FMath::Lerp(From.VignetteIntensity, To.VignetteIntensity, Alpha);
		Result.FilmGrainIntensity = FMath::Lerp(From.FilmGrainIntensity, To.FilmGrainIntensity, Alpha);
		Result.ChromaticAberrationIntensity = FMath::Lerp(From.ChromaticAberrationIntensity, To.ChromaticAberrationIntensity, Alpha);
		Result.BloomIntensity = FMath::Lerp(From.BloomIntensity, To.BloomIntensity, Alpha);
		Result.BloomThreshold = FMath::Lerp(From.BloomThreshold, To.BloomThreshold, Alpha);
		return Result;
	}
}

ASDSelfShotGunActor::ASDSelfShotGunActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(SceneRoot);
	GunMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GunMesh->SetCollisionObjectType(ECC_WorldDynamic);
	GunMesh->SetCollisionResponseToAllChannels(ECR_Block);

	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(SceneRoot);
	InteractionBounds->SetBoxExtent(FVector(38.0f, 16.0f, 10.0f));
	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ChamberPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ChamberPivot"));
	ChamberPivot->SetupAttachment(SceneRoot);

	ChamberMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChamberMesh"));
	ChamberMesh->SetupAttachment(ChamberPivot);
	ChamberMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TriggerPivot = CreateDefaultSubobject<USceneComponent>(TEXT("TriggerPivot"));
	TriggerPivot->SetupAttachment(SceneRoot);

	TriggerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TriggerMesh"));
	TriggerMesh->SetupAttachment(TriggerPivot);
	TriggerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(SceneRoot);
	HandleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RatchetMechanismMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RatchetMechanismMesh"));
	RatchetMechanismMesh->SetupAttachment(SceneRoot);
	RatchetMechanismMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(GunMesh);

	MuzzleFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlashLight"));
	MuzzleFlashLight->SetupAttachment(MuzzlePoint);
	MuzzleFlashLight->SetIntensity(0.0f);
	MuzzleFlashLight->SetAttenuationRadius(MuzzleFlashAttenuationRadius);
	MuzzleFlashLight->SetLightColor(MuzzleFlashColor);

	InitialHitEffectSettings.PixelCount = 95.0f;
	InitialHitEffectSettings.ColorSteps = 2.0f;
	InitialHitEffectSettings.HalftoneStrength = 0.65f;
	InitialHitEffectSettings.HalftoneCount = 120.0f;
	InitialHitEffectSettings.HalftoneRadius = 0.45f;
	InitialHitEffectSettings.HalftoneSoftness = 0.08f;
	InitialHitEffectSettings.HalftoneShape = 0.0f;
	InitialHitEffectSettings.ExposureCompensation = 1.35f;
	InitialHitEffectSettings.Saturation = 0.2f;
	InitialHitEffectSettings.Contrast = 1.45f;
	InitialHitEffectSettings.VignetteIntensity = 0.95f;
	InitialHitEffectSettings.FilmGrainIntensity = 0.9f;
	InitialHitEffectSettings.ChromaticAberrationIntensity = 1.6f;
	InitialHitEffectSettings.BloomIntensity = 1.15f;
	InitialHitEffectSettings.BloomThreshold = 0.3f;

	RecoveryHitEffectSettings.PixelCount = 165.0f;
	RecoveryHitEffectSettings.ColorSteps = 3.0f;
	RecoveryHitEffectSettings.HalftoneStrength = 0.3f;
	RecoveryHitEffectSettings.HalftoneCount = 120.0f;
	RecoveryHitEffectSettings.HalftoneRadius = 0.45f;
	RecoveryHitEffectSettings.HalftoneSoftness = 0.08f;
	RecoveryHitEffectSettings.HalftoneShape = 0.0f;
	RecoveryHitEffectSettings.ExposureCompensation = 0.7f;
	RecoveryHitEffectSettings.Saturation = 0.3f;
	RecoveryHitEffectSettings.Contrast = 1.2f;
	RecoveryHitEffectSettings.VignetteIntensity = 0.85f;
	RecoveryHitEffectSettings.FilmGrainIntensity = 0.55f;
	RecoveryHitEffectSettings.ChromaticAberrationIntensity = 0.8f;
	RecoveryHitEffectSettings.BloomIntensity = 0.35f;
	RecoveryHitEffectSettings.BloomThreshold = 0.3f;
}

void ASDSelfShotGunActor::BeginPlay()
{
	Super::BeginPlay();

	RestActorTransform = GetActorTransform();
	OriginalCollisionEnabled = GunMesh->GetCollisionEnabled();
	MuzzleFlashLight->SetAttenuationRadius(FMath::Max(0.0f, MuzzleFlashAttenuationRadius));
	MuzzleFlashLight->SetLightColor(MuzzleFlashColor);
}

void ASDSelfShotGunActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MuzzleFlashElapsedTime > 0.0f)
	{
		MuzzleFlashElapsedTime = FMath::Max(0.0f, MuzzleFlashElapsedTime - DeltaSeconds);
		MuzzleFlashLight->SetIntensity(
			MuzzleFlashElapsedTime > 0.0f ? MuzzleFlashIntensity : 0.0f);
	}

	UpdateHitSequence(DeltaSeconds);

	if (AnimState == EGunAnimState::Idle)
	{
		return;
	}

	StateElapsedTime += DeltaSeconds;

	switch (AnimState)
	{
	case EGunAnimState::Raising:
	{
		const float Alpha = FMath::Clamp(StateElapsedTime / RaiseTime, 0.0f, 1.0f);
		SetActorTransformAlpha(RaiseStartTransform, GetFirstPersonGunTransform(), FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 3.0f));
		if (Alpha >= 1.0f)
		{
			AnimState = EGunAnimState::Aiming;
			StateElapsedTime = 0.0f;
		}
		break;
	}
	case EGunAnimState::Aiming:
		SetActorTransform(GetFirstPersonGunTransform());
		if (StateElapsedTime >= AimHoldTime)
		{
			FireGun();
		}
		break;
	case EGunAnimState::Fired:
		if (StateElapsedTime >= ShotHoldTime)
		{
			ReturnStartTransform = GetActorTransform();
			AnimState = EGunAnimState::Returning;
			StateElapsedTime = 0.0f;

		}
		break;
	case EGunAnimState::Returning:
	{
		const float Alpha = FMath::Clamp(StateElapsedTime / ReturnTime, 0.0f, 1.0f);
		SetActorTransformAlpha(ReturnStartTransform, RestActorTransform, FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 2.0f));
		if (Alpha >= 1.0f)
		{
			FinishSequence();
		}
		break;
	}
	default:
		break;
	}
}

void ASDSelfShotGunActor::UseGun()
{
	if (!CanInteract_Implementation(nullptr))
	{
		return;
	}

	RestActorTransform = GetActorTransform();
	RaiseStartTransform = RestActorTransform;
	StateElapsedTime = 0.0f;
	AnimState = EGunAnimState::Raising;

	if (bDisableCollisionWhileUsing)
	{
		OriginalCollisionEnabled = GunMesh->GetCollisionEnabled();
		GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	OnGunRaised.Broadcast();
}

bool ASDSelfShotGunActor::CanInteract_Implementation(AActor* Interactor) const
{
	return AnimState == EGunAnimState::Idle && HitSequenceState == EHitSequenceState::Idle;
}

void ASDSelfShotGunActor::Interact_Implementation(AActor* Interactor)
{
	UseGun();
}

void ASDSelfShotGunActor::FireGun()
{
	AnimState = EGunAnimState::Fired;
	StateElapsedTime = 0.0f;
	MuzzleFlashElapsedTime = MuzzleFlashDuration;
	MuzzleFlashLight->SetAttenuationRadius(FMath::Max(0.0f, MuzzleFlashAttenuationRadius));
	MuzzleFlashLight->SetLightColor(MuzzleFlashColor);
	MuzzleFlashLight->SetIntensity(MuzzleFlashIntensity);

	if (GunshotSound)
	{
		if (bPlayGunshotSound2D)
		{
			UGameplayStatics::PlaySound2D(this, GunshotSound);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(this, GunshotSound, GetActorLocation());
		}
	}

	if (bEnableHitSequence)
	{
		StartHitSequence();
	}
	OnGunFired.Broadcast();
}

void ASDSelfShotGunActor::FinishSequence()
{
	SetActorTransform(RestActorTransform);
	GunMesh->SetCollisionEnabled(OriginalCollisionEnabled);
	MuzzleFlashLight->SetIntensity(0.0f);
	MuzzleFlashElapsedTime = 0.0f;
	StateElapsedTime = 0.0f;
	AnimState = EGunAnimState::Idle;
	OnGunSequenceFinished.Broadcast();
}

void ASDSelfShotGunActor::StartHitSequence()
{
	ASDArtToneController* Controller = ResolveHitSequenceArtToneController();
	if (Controller)
	{
		HitSequenceBaseSettings = Controller->GetCurrentSettings();
		Controller->ApplyArtTone(InitialHitEffectSettings);
	}

	if (AShowDownPlayerController* ShowDownController = Cast<AShowDownPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		ShowDownController->PlayFixedCameraSteppedShake(
			InitialHitEffectDuration,
			0.0f,
			InitialHitShakeRotationAmplitude,
			InitialHitShakeLocationAmplitude,
			InitialHitShakeStepInterval);
	}

	HitSequenceState = EHitSequenceState::InitialHit;
	HitSequenceElapsedTime = 0.0f;
	if (InitialHitEffectDuration <= KINDA_SMALL_NUMBER)
	{
		EnterHitSequenceBlackout();
	}
}

void ASDSelfShotGunActor::UpdateHitSequence(float DeltaSeconds)
{
	if (HitSequenceState == EHitSequenceState::Idle)
	{
		return;
	}

	HitSequenceElapsedTime += DeltaSeconds;

	switch (HitSequenceState)
	{
	case EHitSequenceState::InitialHit:
		if (HitSequenceElapsedTime >= InitialHitEffectDuration)
		{
			EnterHitSequenceBlackout();
		}
		break;
	case EHitSequenceState::Blackout:
		if (HitSequenceElapsedTime >= HitBlackoutDuration)
		{
			EnterHitSequenceRecovery();
		}
		break;
	case EHitSequenceState::RecoveryHold:
		if (HitSequenceElapsedTime >= RecoveryHitEffectHoldTime)
		{
			HitSequenceState = EHitSequenceState::RecoveryBlendOut;
		}
		break;
	case EHitSequenceState::RecoveryBlendOut:
	{
		ASDArtToneController* Controller = ResolveHitSequenceArtToneController();
		const float EffectBlendElapsed = FMath::Max(0.0f, HitSequenceElapsedTime - RecoveryHitEffectHoldTime);
		const float EffectBlendAlpha = RecoveryHitEffectBlendOutTime > KINDA_SMALL_NUMBER
			? FMath::Clamp(EffectBlendElapsed / RecoveryHitEffectBlendOutTime, 0.0f, 1.0f)
			: 1.0f;
		const float EasedAlpha = ApplyEase(
			EffectBlendAlpha,
			RecoveryHitEffectEaseMode,
			RecoveryHitEffectEaseExponent);
		if (Controller)
		{
			Controller->ApplyArtTone(LerpArtToneSettings(RecoveryHitEffectSettings, HitSequenceBaseSettings, EasedAlpha));
		}

		const float EffectTotalTime = RecoveryHitEffectHoldTime + RecoveryHitEffectBlendOutTime;
		const float ShakeTotalTime = RecoveryHitShakeHoldTime + RecoveryHitShakeBlendOutTime;
		if (HitSequenceElapsedTime >= FMath::Max(EffectTotalTime, ShakeTotalTime))
		{
			FinishHitSequence();
		}
		break;
	}
	default:
		break;
	}
}

void ASDSelfShotGunActor::EnterHitSequenceBlackout()
{
	SetBlackoutInstant(FMath::Clamp(HitBlackoutAmount, 0.0f, 1.0f), true);
	HitSequenceState = EHitSequenceState::Blackout;
	HitSequenceElapsedTime = 0.0f;
}

void ASDSelfShotGunActor::EnterHitSequenceRecovery()
{
	ASDArtToneController* Controller = ResolveHitSequenceArtToneController();
	if (Controller)
	{
		Controller->ApplyArtTone(RecoveryHitEffectSettings);
	}

	SetBlackoutInstant(0.0f, false);
	if (AShowDownPlayerController* ShowDownController = Cast<AShowDownPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		ShowDownController->PlayFixedCameraSteppedShake(
			RecoveryHitShakeHoldTime,
			RecoveryHitShakeBlendOutTime,
			RecoveryHitShakeRotationAmplitude,
			RecoveryHitShakeLocationAmplitude,
			RecoveryHitShakeStepInterval);
	}

	HitSequenceState = RecoveryHitEffectHoldTime > KINDA_SMALL_NUMBER
		? EHitSequenceState::RecoveryHold
		: EHitSequenceState::RecoveryBlendOut;
	HitSequenceElapsedTime = 0.0f;
}

void ASDSelfShotGunActor::FinishHitSequence()
{
	if (ASDArtToneController* Controller = ResolveHitSequenceArtToneController())
	{
		Controller->ApplyArtTone(HitSequenceBaseSettings);
	}

	SetBlackoutInstant(0.0f, false);
	HitSequenceState = EHitSequenceState::Idle;
	HitSequenceElapsedTime = 0.0f;
}

void ASDSelfShotGunActor::SetBlackoutInstant(float Alpha, bool bHoldWhenFinished)
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(
				Alpha > 0.0f ? 0.0f : FMath::Clamp(HitBlackoutAmount, 0.0f, 1.0f),
				Alpha,
				0.001f,
				FLinearColor::Black,
				false,
				bHoldWhenFinished);
		}
	}
}

ASDArtToneController* ASDSelfShotGunActor::ResolveHitSequenceArtToneController()
{
	if (HitSequenceArtToneController)
	{
		return HitSequenceArtToneController;
	}

	HitSequenceArtToneController = Cast<ASDArtToneController>(
		UGameplayStatics::GetActorOfClass(this, ASDArtToneController::StaticClass()));
	return HitSequenceArtToneController;
}

FTransform ASDSelfShotGunActor::GetFirstPersonGunTransform() const
{
	if (!bUseFirstPersonPose)
	{
		return RestActorTransform;
	}

	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
		{
			const FRotator CameraRotation = CameraManager->GetCameraRotation();
			const FRotationMatrix CameraMatrix(CameraRotation);
			const FVector CameraLocation = CameraManager->GetCameraLocation();
			const FVector TargetLocation = CameraLocation
				+ CameraMatrix.GetScaledAxis(EAxis::X) * FirstPersonCameraOffset.X
				+ CameraMatrix.GetScaledAxis(EAxis::Y) * FirstPersonCameraOffset.Y
				+ CameraMatrix.GetScaledAxis(EAxis::Z) * FirstPersonCameraOffset.Z;

			return FTransform(CameraRotation + FirstPersonRotationOffset, TargetLocation, RestActorTransform.GetScale3D());
		}
	}

	return RestActorTransform;
}

void ASDSelfShotGunActor::SetActorTransformAlpha(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha)
{
	const FVector NewLocation = FMath::Lerp(FromTransform.GetLocation(), ToTransform.GetLocation(), Alpha);
	const FQuat NewRotation = FQuat::Slerp(FromTransform.GetRotation(), ToTransform.GetRotation(), Alpha);
	const FVector NewScale = FMath::Lerp(FromTransform.GetScale3D(), ToTransform.GetScale3D(), Alpha);
	SetActorTransform(FTransform(NewRotation, NewLocation, NewScale));
}
