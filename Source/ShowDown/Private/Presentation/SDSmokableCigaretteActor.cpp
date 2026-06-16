// Fill out your copyright notice in the Description page of Project Settings.

#include "Presentation/SDSmokableCigaretteActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"

ASDSmokableCigaretteActor::ASDSmokableCigaretteActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CigaretteMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CigaretteMesh"));
	CigaretteMesh->SetupAttachment(SceneRoot);
	CigaretteMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CigaretteMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CigaretteMesh->SetCollisionResponseToAllChannels(ECR_Block);

	EmberLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EmberLight"));
	EmberLight->SetupAttachment(CigaretteMesh);
	EmberLight->SetIntensity(EmberIdleIntensity);
	EmberLight->SetAttenuationRadius(35.0f);
	EmberLight->SetLightColor(FLinearColor(1.0f, 0.18f, 0.05f));

	SmokeParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SmokeParticle"));
	SmokeParticle->SetupAttachment(CigaretteMesh);
	SmokeParticle->SetAutoActivate(false);
}

void ASDSmokableCigaretteActor::BeginPlay()
{
	Super::BeginPlay();

	RestRelativeLocation = CigaretteMesh->GetRelativeLocation();
	RestRelativeRotation = CigaretteMesh->GetRelativeRotation();
	SmokeRelativeLocation = RestRelativeLocation + LocalSmokeOffset;
	SmokeRelativeRotation = RestRelativeRotation + LocalSmokeRotationOffset;
	RestActorTransform = GetActorTransform();
	OriginalCollisionEnabled = CigaretteMesh->GetCollisionEnabled();

	EmberLight->SetIntensity(EmberIdleIntensity);
	SmokeParticle->DeactivateSystem();

	if (bEnableMouseClick)
	{
		CigaretteMesh->OnClicked.AddDynamic(this, &ASDSmokableCigaretteActor::HandleCigaretteClicked);
	}

	if (bEnablePlayerClickEventsOnBeginPlay)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			PlayerController->bEnableClickEvents = true;
		}
	}
}

void ASDSmokableCigaretteActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (AnimState == ECigaretteAnimState::Idle)
	{
		return;
	}

	StateElapsedTime += DeltaSeconds;
	SmokeElapsedTime += DeltaSeconds;
	UpdateEmber(DeltaSeconds);

	switch (AnimState)
	{
	case ECigaretteAnimState::Lifting:
	{
		const float Alpha = FMath::Clamp(StateElapsedTime / LiftTime, 0.0f, 1.0f);
		const float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 3.0f);

		if (bUseFirstPersonSmokePose)
		{
			SetActorTransformAlpha(SmokeStartActorTransform, GetFirstPersonSmokeTransform(), EasedAlpha);
		}
		else
		{
			SetCigarettePoseAlpha(EasedAlpha);
		}

		if (Alpha >= 1.0f)
		{
			AnimState = ECigaretteAnimState::Holding;
			StateElapsedTime = 0.0f;
		}
		break;
	}
	case ECigaretteAnimState::Holding:
		if (bUseFirstPersonSmokePose)
		{
			SetActorTransform(GetFirstPersonSmokeTransform());
		}

		if (StateElapsedTime >= SmokeHoldTime)
		{
			ReturnStartActorTransform = GetActorTransform();
			AnimState = ECigaretteAnimState::Returning;
			StateElapsedTime = 0.0f;
		}
		break;
	case ECigaretteAnimState::Returning:
	{
		const float Alpha = FMath::Clamp(StateElapsedTime / ReturnTime, 0.0f, 1.0f);
		const float EasedAlpha = FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 2.0f);

		if (bUseFirstPersonSmokePose)
		{
			SetActorTransformAlpha(ReturnStartActorTransform, RestActorTransform, EasedAlpha);
		}
		else
		{
			SetCigarettePoseAlpha(1.0f - EasedAlpha);
		}

		if (Alpha >= 1.0f)
		{
			FinishSmoke();
		}
		break;
	}
	default:
		break;
	}
}

void ASDSmokableCigaretteActor::Smoke()
{
	if (!CanInteract_Implementation(nullptr))
	{
		return;
	}

	SmokeRelativeLocation = RestRelativeLocation + LocalSmokeOffset;
	SmokeRelativeRotation = RestRelativeRotation + LocalSmokeRotationOffset;
	RestActorTransform = GetActorTransform();
	SmokeStartActorTransform = RestActorTransform;
	ReturnStartActorTransform = RestActorTransform;
	AnimState = ECigaretteAnimState::Lifting;
	StateElapsedTime = 0.0f;
	SmokeElapsedTime = 0.0f;

	if (bDisableCollisionWhileSmoking)
	{
		OriginalCollisionEnabled = CigaretteMesh->GetCollisionEnabled();
		CigaretteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	EmberLight->SetIntensity(EmberSmokeIntensity);
	SmokeParticle->ActivateSystem(true);

	if (SmokeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SmokeSound, GetActorLocation());
	}

	OnSmokeStarted.Broadcast();
}

bool ASDSmokableCigaretteActor::CanInteract_Implementation(AActor* Interactor) const
{
	return AnimState == ECigaretteAnimState::Idle;
}

void ASDSmokableCigaretteActor::Interact_Implementation(AActor* Interactor)
{
	Smoke();
}

void ASDSmokableCigaretteActor::HandleCigaretteClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	Smoke();
}

void ASDSmokableCigaretteActor::SetCigarettePoseAlpha(float Alpha)
{
	CigaretteMesh->SetRelativeLocation(FMath::Lerp(RestRelativeLocation, SmokeRelativeLocation, Alpha));

	const FQuat RestQuat = RestRelativeRotation.Quaternion();
	const FQuat SmokeQuat = SmokeRelativeRotation.Quaternion();
	CigaretteMesh->SetRelativeRotation(FQuat::Slerp(RestQuat, SmokeQuat, Alpha));
}

FTransform ASDSmokableCigaretteActor::GetFirstPersonSmokeTransform() const
{
	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
		{
			const FVector CameraLocation = CameraManager->GetCameraLocation();
			const FRotator CameraRotation = CameraManager->GetCameraRotation();
			const FRotationMatrix CameraMatrix(CameraRotation);

			const FVector TargetLocation =
				CameraLocation +
				CameraMatrix.GetScaledAxis(EAxis::X) * FirstPersonCameraOffset.X +
				CameraMatrix.GetScaledAxis(EAxis::Y) * FirstPersonCameraOffset.Y +
				CameraMatrix.GetScaledAxis(EAxis::Z) * FirstPersonCameraOffset.Z;

			const FRotator TargetRotation = CameraRotation + FirstPersonRotationOffset;
			return FTransform(TargetRotation, TargetLocation, RestActorTransform.GetScale3D());
		}
	}

	return RestActorTransform;
}

void ASDSmokableCigaretteActor::SetActorTransformAlpha(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha)
{
	const FVector NewLocation = FMath::Lerp(FromTransform.GetLocation(), ToTransform.GetLocation(), Alpha);
	const FQuat NewRotation = FQuat::Slerp(FromTransform.GetRotation(), ToTransform.GetRotation(), Alpha);
	const FVector NewScale = FMath::Lerp(FromTransform.GetScale3D(), ToTransform.GetScale3D(), Alpha);

	SetActorTransform(FTransform(NewRotation, NewLocation, NewScale));
}

void ASDSmokableCigaretteActor::UpdateEmber(float DeltaSeconds)
{
	const float Flicker = FMath::Sin(SmokeElapsedTime * EmberFlickerSpeed) * EmberFlickerAmount;
	const float TargetIntensity = FMath::Max(0.0f, EmberSmokeIntensity + Flicker);
	EmberLight->SetIntensity(TargetIntensity);
}

void ASDSmokableCigaretteActor::FinishSmoke()
{
	if (bUseFirstPersonSmokePose)
	{
		SetActorTransform(RestActorTransform);
	}
	else
	{
		SetCigarettePoseAlpha(0.0f);
	}

	if (bDisableCollisionWhileSmoking)
	{
		CigaretteMesh->SetCollisionEnabled(OriginalCollisionEnabled);
	}

	EmberLight->SetIntensity(EmberIdleIntensity);
	SmokeParticle->DeactivateSystem();
	AnimState = ECigaretteAnimState::Idle;
	StateElapsedTime = 0.0f;
	SmokeElapsedTime = 0.0f;
	OnSmokeFinished.Broadcast();
}
