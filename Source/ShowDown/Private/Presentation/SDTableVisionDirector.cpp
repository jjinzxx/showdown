#include "Presentation/SDTableVisionDirector.h"

#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* DefaultDarknessMaterialPath = TEXT("/Game/ArtTone/M_PP_TableVisionWorldRange.M_PP_TableVisionWorldRange");
	const TCHAR* DeprecatedDarknessMaterialPath = TEXT("/Game/ArtTone/M_PP_TableVisionDarkness.M_PP_TableVisionDarkness");
	const FName DarknessStrengthParameterName(TEXT("DarknessStrength"));
	const FName VisionCenterParameterName(TEXT("VisionCenter"));
	const FName VisionRadiusParameterName(TEXT("VisionRadius"));
	const FName VisionFeatherParameterName(TEXT("VisionFeather"));
}

ASDTableVisionDirector::ASDTableVisionDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(SceneRoot);
	PostProcessComponent->bUnbound = bUnboundPostProcess;
	PostProcessComponent->Priority = PostProcessPriority;
	PostProcessComponent->BlendWeight = 1.0f;

	TableSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("TableSpotLight"));
	TableSpotLight->SetupAttachment(SceneRoot);
	TableSpotLight->Mobility = EComponentMobility::Movable;
	TableSpotLight->SetRelativeLocation(FVector(0.0f, 0.0f, OverheadLightHeight));
	TableSpotLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TableSpotLight->SetCastShadows(true);
	TableSpotLight->SetInnerConeAngle(SpotLightInnerConeAngle);
	TableSpotLight->SetOuterConeAngle(SpotLightOuterConeAngle);

	TableFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TableFillLight"));
	TableFillLight->SetupAttachment(SceneRoot);
	TableFillLight->Mobility = EComponentMobility::Movable;
	TableFillLight->SetRelativeLocation(FVector::ZeroVector);
	TableFillLight->SetCastShadows(false);

	CollapsedVision.VisionRadius = 450.0f;
	CollapsedVision.VisionFeather = 120.0f;
	CollapsedVision.DarknessStrength = 1.0f;
	CollapsedVision.SpotLightIntensity = 6500.0f;
	CollapsedVision.SpotLightAttenuationRadius = 520.0f;
	CollapsedVision.PointLightIntensity = 0.0f;
	CollapsedVision.PointLightAttenuationRadius = 420.0f;
	CollapsedVision.ExposureCompensation = -1.2f;
	CollapsedVision.VignetteIntensity = 0.85f;
	CollapsedVision.BloomIntensity = 0.12f;

	RevealedVision.VisionRadius = 1800.0f;
	RevealedVision.VisionFeather = 360.0f;
	RevealedVision.DarknessStrength = 0.25f;
	RevealedVision.SpotLightIntensity = 11500.0f;
	RevealedVision.SpotLightAttenuationRadius = 1800.0f;
	RevealedVision.PointLightIntensity = 700.0f;
	RevealedVision.PointLightAttenuationRadius = 1400.0f;
	RevealedVision.ExposureCompensation = -0.35f;
	RevealedVision.VignetteIntensity = 0.38f;
	RevealedVision.BloomIntensity = 0.25f;

	CurrentState = CollapsedVision;
}

void ASDTableVisionDirector::BeginPlay()
{
	Super::BeginPlay();

	PostProcessComponent->bUnbound = bUnboundPostProcess;
	PostProcessComponent->Priority = PostProcessPriority;
	UpdateLightTransform();

	if (bApplyInitialStateOnBeginPlay)
	{
		ApplyCollapsedVision();
	}

	if (bPlayRevealOnBeginPlay)
	{
		PlayRevealPulse();
	}
	else
	{
		UpdateTickState();
	}
}

void ASDTableVisionDirector::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	PostProcessComponent->bUnbound = bUnboundPostProcess;
	PostProcessComponent->Priority = PostProcessPriority;
	UpdateLightTransform();

	CurrentRevealAlpha = EditorPreviewRevealAlpha;
	CurrentState = LerpVisionState(CollapsedVision, RevealedVision, CurrentRevealAlpha);
	ApplyCurrentState();
}

void ASDTableVisionDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bRevealPulseActive)
	{
		UpdateRevealPulse(DeltaSeconds);
	}
	else if (bTrackVisionCenterEveryTick)
	{
		ApplyDarknessMaterialState();
	}

	UpdateTickState();
}

#if WITH_EDITOR
void ASDTableVisionDirector::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASDTableVisionDirector, DarknessPostProcessMaterial))
	{
		DarknessMID = nullptr;
	}

	PostProcessComponent->bUnbound = bUnboundPostProcess;
	PostProcessComponent->Priority = PostProcessPriority;
	UpdateLightTransform();
	CurrentRevealAlpha = EditorPreviewRevealAlpha;
	CurrentState = LerpVisionState(CollapsedVision, RevealedVision, CurrentRevealAlpha);
	ApplyCurrentState();
}
#endif

void ASDTableVisionDirector::ApplyCollapsedVision()
{
	bRevealPulseActive = false;
	SetRevealAlpha(0.0f);
}

void ASDTableVisionDirector::ApplyRevealedVision()
{
	bRevealPulseActive = false;
	SetRevealAlpha(1.0f);
}

void ASDTableVisionDirector::SetRevealAlpha(float Alpha)
{
	CurrentRevealAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	CurrentState = LerpVisionState(CollapsedVision, RevealedVision, CurrentRevealAlpha);
	ApplyCurrentState();
	UpdateTickState();
}

void ASDTableVisionDirector::PlayRevealPulse(float ExpandTime, float HoldTime, float CollapseTime)
{
	ActiveExpandTime = ExpandTime >= 0.0f ? ExpandTime : RevealExpandTime;
	ActiveHoldTime = HoldTime >= 0.0f ? HoldTime : RevealHoldTime;
	ActiveCollapseTime = CollapseTime >= 0.0f ? CollapseTime : RevealCollapseTime;
	RevealPulseElapsed = 0.0f;
	bRevealPulseActive = true;
	SetActorTickEnabled(true);
	SetRevealAlpha(0.0f);
}

void ASDTableVisionDirector::RevealSurroundings(float ExpandTime, float HoldTime, float CollapseTime)
{
	PlayRevealPulse(ExpandTime, HoldTime, CollapseTime);
}

void ASDTableVisionDirector::StopRevealPulse(bool bSnapToCollapsed)
{
	bRevealPulseActive = false;
	if (bSnapToCollapsed)
	{
		SetRevealAlpha(0.0f);
	}
	UpdateTickState();
}

void ASDTableVisionDirector::EnsureDarknessMaterialInstance()
{
	if (DarknessPostProcessMaterial && DarknessPostProcessMaterial->GetPathName() == DeprecatedDarknessMaterialPath)
	{
		DarknessPostProcessMaterial = nullptr;
		DarknessMID = nullptr;
	}

	if (!DarknessPostProcessMaterial && bAutoLoadDefaultDarknessMaterial)
	{
		DarknessPostProcessMaterial = LoadObject<UMaterialInterface>(nullptr, DefaultDarknessMaterialPath);
	}

	if (!DarknessPostProcessMaterial || !PostProcessComponent)
	{
		return;
	}

	if (!DarknessMID)
	{
		DarknessMID = UMaterialInstanceDynamic::Create(DarknessPostProcessMaterial, this);
	}

	if (!DarknessMID)
	{
		return;
	}

	FWeightedBlendables& WeightedBlendables = PostProcessComponent->Settings.WeightedBlendables;
	for (int32 Index = WeightedBlendables.Array.Num() - 1; Index >= 0; --Index)
	{
		const UObject* BlendableObject = WeightedBlendables.Array[Index].Object.Get();
		if (IsMatchingBlendable(BlendableObject, DarknessPostProcessMaterial, DarknessMID))
		{
			WeightedBlendables.Array.RemoveAt(Index);
		}
	}

	WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, DarknessMID));
}

void ASDTableVisionDirector::ApplyCurrentState()
{
	ApplyLightState();
	ApplyPostProcessState();
	ApplyDarknessMaterialState();
}

void ASDTableVisionDirector::ApplyLightState()
{
	if (TableSpotLight)
	{
		TableSpotLight->SetLightColor(LightColor);
		TableSpotLight->SetIntensity(FMath::Max(0.0f, CurrentState.SpotLightIntensity));
		TableSpotLight->SetAttenuationRadius(FMath::Max(0.0f, CurrentState.SpotLightAttenuationRadius));
		TableSpotLight->SetInnerConeAngle(SpotLightInnerConeAngle);
		TableSpotLight->SetOuterConeAngle(SpotLightOuterConeAngle);
	}

	if (TableFillLight)
	{
		TableFillLight->SetVisibility(bUsePointFillLight);
		TableFillLight->SetLightColor(LightColor);
		TableFillLight->SetIntensity(bUsePointFillLight ? FMath::Max(0.0f, CurrentState.PointLightIntensity) : 0.0f);
		TableFillLight->SetAttenuationRadius(FMath::Max(0.0f, CurrentState.PointLightAttenuationRadius));
	}
}

void ASDTableVisionDirector::ApplyPostProcessState()
{
	if (!PostProcessComponent)
	{
		return;
	}

	if (!ShouldApplyPostProcessInCurrentWorld())
	{
		PostProcessComponent->BlendWeight = 0.0f;
		return;
	}

	PostProcessComponent->BlendWeight = 1.0f;
	FPostProcessSettings& PostSettings = PostProcessComponent->Settings;

	PostSettings.bOverride_AutoExposureBias = true;
	PostSettings.AutoExposureBias = CurrentState.ExposureCompensation;

	PostSettings.bOverride_VignetteIntensity = true;
	PostSettings.VignetteIntensity = CurrentState.VignetteIntensity;

	PostSettings.bOverride_BloomIntensity = true;
	PostSettings.BloomIntensity = CurrentState.BloomIntensity;
}

void ASDTableVisionDirector::ApplyDarknessMaterialState()
{
	if (!PostProcessComponent)
	{
		return;
	}

	if (!bEnableDarknessPostProcess || !ShouldApplyPostProcessInCurrentWorld())
	{
		PostProcessComponent->Settings.WeightedBlendables.Array.Empty();
		return;
	}

	EnsureDarknessMaterialInstance();
	if (!DarknessMID)
	{
		return;
	}

	const FVector VisionCenter = GetVisionCenterWorldLocation();
	DarknessMID->SetVectorParameterValue(
		VisionCenterParameterName,
		FLinearColor(VisionCenter.X, VisionCenter.Y, VisionCenter.Z, 1.0f));
	DarknessMID->SetScalarParameterValue(DarknessStrengthParameterName, FMath::Clamp(CurrentState.DarknessStrength, 0.0f, 1.0f));
	DarknessMID->SetScalarParameterValue(VisionRadiusParameterName, FMath::Max(0.0f, CurrentState.VisionRadius));
	DarknessMID->SetScalarParameterValue(VisionFeatherParameterName, FMath::Max(1.0f, CurrentState.VisionFeather));
}

void ASDTableVisionDirector::UpdateLightTransform()
{
	if (TableSpotLight)
	{
		TableSpotLight->SetRelativeLocation(FVector(0.0f, 0.0f, OverheadLightHeight));
		TableSpotLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	}

	if (TableFillLight)
	{
		TableFillLight->SetRelativeLocation(FVector::ZeroVector);
	}
}

void ASDTableVisionDirector::UpdateTickState()
{
	SetActorTickEnabled(bRevealPulseActive || bTrackVisionCenterEveryTick);
}

void ASDTableVisionDirector::UpdateRevealPulse(float DeltaSeconds)
{
	RevealPulseElapsed += DeltaSeconds;

	const float ExpandTime = FMath::Max(0.0f, ActiveExpandTime);
	const float HoldTime = FMath::Max(0.0f, ActiveHoldTime);
	const float CollapseTime = FMath::Max(0.0f, ActiveCollapseTime);
	const float TotalTime = ExpandTime + HoldTime + CollapseTime;

	if (TotalTime <= KINDA_SMALL_NUMBER)
	{
		bRevealPulseActive = false;
		SetRevealAlpha(0.0f);
		return;
	}

	float Alpha = 0.0f;
	if (ExpandTime > KINDA_SMALL_NUMBER && RevealPulseElapsed < ExpandTime)
	{
		Alpha = EaseUnitAlpha(RevealPulseElapsed / ExpandTime);
	}
	else if (RevealPulseElapsed < ExpandTime + HoldTime)
	{
		Alpha = 1.0f;
	}
	else if (CollapseTime > KINDA_SMALL_NUMBER && RevealPulseElapsed < TotalTime)
	{
		const float CollapseAlpha = (RevealPulseElapsed - ExpandTime - HoldTime) / CollapseTime;
		Alpha = 1.0f - EaseUnitAlpha(CollapseAlpha);
	}
	else
	{
		bRevealPulseActive = false;
		Alpha = 0.0f;
	}

	SetRevealAlpha(Alpha);
}

FVector ASDTableVisionDirector::GetVisionCenterWorldLocation() const
{
	return VisionCenterActor ? VisionCenterActor->GetActorLocation() : GetActorLocation();
}

bool ASDTableVisionDirector::ShouldApplyPostProcessInCurrentWorld() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	return World->IsGameWorld() || bPreviewPostProcessInEditor;
}

float ASDTableVisionDirector::EaseUnitAlpha(float Alpha) const
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	return FMath::InterpEaseInOut(0.0f, 1.0f, ClampedAlpha, FMath::Max(0.01f, RevealEaseExponent));
}

FSDTableVisionState ASDTableVisionDirector::LerpVisionState(
	const FSDTableVisionState& From,
	const FSDTableVisionState& To,
	float Alpha)
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	FSDTableVisionState Result;
	Result.VisionRadius = FMath::Lerp(From.VisionRadius, To.VisionRadius, ClampedAlpha);
	Result.VisionFeather = FMath::Lerp(From.VisionFeather, To.VisionFeather, ClampedAlpha);
	Result.DarknessStrength = FMath::Lerp(From.DarknessStrength, To.DarknessStrength, ClampedAlpha);
	Result.SpotLightIntensity = FMath::Lerp(From.SpotLightIntensity, To.SpotLightIntensity, ClampedAlpha);
	Result.SpotLightAttenuationRadius = FMath::Lerp(From.SpotLightAttenuationRadius, To.SpotLightAttenuationRadius, ClampedAlpha);
	Result.PointLightIntensity = FMath::Lerp(From.PointLightIntensity, To.PointLightIntensity, ClampedAlpha);
	Result.PointLightAttenuationRadius = FMath::Lerp(From.PointLightAttenuationRadius, To.PointLightAttenuationRadius, ClampedAlpha);
	Result.ExposureCompensation = FMath::Lerp(From.ExposureCompensation, To.ExposureCompensation, ClampedAlpha);
	Result.VignetteIntensity = FMath::Lerp(From.VignetteIntensity, To.VignetteIntensity, ClampedAlpha);
	Result.BloomIntensity = FMath::Lerp(From.BloomIntensity, To.BloomIntensity, ClampedAlpha);
	return Result;
}

bool ASDTableVisionDirector::IsMatchingBlendable(
	const UObject* BlendableObject,
	const UMaterialInterface* Material,
	const UMaterialInstanceDynamic* MID)
{
	if (!BlendableObject)
	{
		return false;
	}

	if (BlendableObject == Material || BlendableObject == MID)
	{
		return true;
	}

	const UMaterialInterface* BlendableMaterial = Cast<UMaterialInterface>(BlendableObject);
	return BlendableMaterial && Material && BlendableMaterial->GetMaterial() == Material->GetMaterial();
}
