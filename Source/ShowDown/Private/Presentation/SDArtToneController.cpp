// Fill out your copyright notice in the Description page of Project Settings.

#include "Presentation/SDArtToneController.h"

#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ASDArtToneController::ASDArtToneController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	SetRootComponent(PostProcessComponent);
	PostProcessComponent->bUnbound = bUnbound;
	PostProcessComponent->Priority = 50.0f;
	PostProcessComponent->BlendWeight = 1.0f;

	CurrentSettings = InitialSettings;
	BlendStartSettings = InitialSettings;
	BlendTargetSettings = InitialSettings;
}

void ASDArtToneController::BeginPlay()
{
	Super::BeginPlay();

	PostProcessComponent->bUnbound = bUnbound;
	EnsurePixelateMaterialInstance();

	if (bApplyInitialSettingsOnBeginPlay)
	{
		ApplyArtTone(InitialSettings);
	}
}

void ASDArtToneController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsBlending)
	{
		SetActorTickEnabled(false);
		return;
	}

	BlendElapsedTime += DeltaSeconds;
	const float Alpha = BlendDuration <= 0.0f ? 1.0f : FMath::Clamp(BlendElapsedTime / BlendDuration, 0.0f, 1.0f);
	const float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

	ApplyArtTone(LerpSettings(BlendStartSettings, BlendTargetSettings, EasedAlpha));

	if (Alpha >= 1.0f)
	{
		bIsBlending = false;
		BlendElapsedTime = 0.0f;
		BlendDuration = 0.0f;
		SetActorTickEnabled(false);
	}
}

void ASDArtToneController::ApplyArtTone(const FSDArtToneSettings& NewSettings)
{
	CurrentSettings = NewSettings;
	ApplyPostProcessSettings(CurrentSettings);
	ApplyPixelateParameters(CurrentSettings);
}

void ASDArtToneController::BlendToArtTone(const FSDArtToneSettings& NewSettings, float Duration)
{
	if (Duration <= 0.0f)
	{
		ApplyArtTone(NewSettings);
		bIsBlending = false;
		SetActorTickEnabled(false);
		return;
	}

	BlendStartSettings = CurrentSettings;
	BlendTargetSettings = NewSettings;
	BlendDuration = Duration;
	BlendElapsedTime = 0.0f;
	bIsBlending = true;
	SetActorTickEnabled(true);
}

void ASDArtToneController::SetPixelCount(float NewPixelCount)
{
	CurrentSettings.PixelCount = FMath::Max(1.0f, NewPixelCount);
	ApplyPixelateParameters(CurrentSettings);
}

void ASDArtToneController::SetScanlineStrength(float NewScanlineStrength)
{
	CurrentSettings.ScanlineStrength = FMath::Max(0.0f, NewScanlineStrength);
	ApplyPixelateParameters(CurrentSettings);
}

void ASDArtToneController::SetColorSteps(float NewColorSteps)
{
	CurrentSettings.ColorSteps = FMath::Max(1.0f, NewColorSteps);
	ApplyPixelateParameters(CurrentSettings);
}

void ASDArtToneController::EnsurePixelateMaterialInstance()
{
	if (PixelateMID || !PixelatePostProcessMaterial)
	{
		return;
	}

	PixelateMID = UMaterialInstanceDynamic::Create(PixelatePostProcessMaterial, this);
	if (PixelateMID)
	{
		PostProcessComponent->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, PixelateMID));
	}
}

void ASDArtToneController::ApplyPostProcessSettings(const FSDArtToneSettings& Settings)
{
	FPostProcessSettings& PostSettings = PostProcessComponent->Settings;

	PostSettings.bOverride_AutoExposureBias = true;
	PostSettings.AutoExposureBias = Settings.ExposureCompensation;

	PostSettings.bOverride_ColorSaturation = true;
	PostSettings.ColorSaturation = FVector4(Settings.Saturation, Settings.Saturation, Settings.Saturation, 1.0f);

	PostSettings.bOverride_ColorContrast = true;
	PostSettings.ColorContrast = FVector4(Settings.Contrast, Settings.Contrast, Settings.Contrast, 1.0f);

	PostSettings.bOverride_VignetteIntensity = true;
	PostSettings.VignetteIntensity = Settings.VignetteIntensity;

	PostSettings.bOverride_FilmGrainIntensity = true;
	PostSettings.FilmGrainIntensity = Settings.FilmGrainIntensity;

	PostSettings.bOverride_SceneFringeIntensity = true;
	PostSettings.SceneFringeIntensity = Settings.ChromaticAberrationIntensity;

	PostSettings.bOverride_BloomIntensity = true;
	PostSettings.BloomIntensity = Settings.BloomIntensity;

	PostSettings.bOverride_BloomThreshold = true;
	PostSettings.BloomThreshold = Settings.BloomThreshold;
}

void ASDArtToneController::ApplyPixelateParameters(const FSDArtToneSettings& Settings)
{
	EnsurePixelateMaterialInstance();

	if (!PixelateMID)
	{
		return;
	}

	PixelateMID->SetScalarParameterValue(TEXT("PixelCount"), Settings.PixelCount);
	PixelateMID->SetScalarParameterValue(TEXT("ScanlineStrength"), Settings.ScanlineStrength);
	PixelateMID->SetScalarParameterValue(TEXT("ColorSteps"), Settings.ColorSteps);
}

FSDArtToneSettings ASDArtToneController::LerpSettings(const FSDArtToneSettings& From, const FSDArtToneSettings& To, float Alpha)
{
	FSDArtToneSettings Result;
	Result.PixelCount = FMath::Lerp(From.PixelCount, To.PixelCount, Alpha);
	Result.ScanlineStrength = FMath::Lerp(From.ScanlineStrength, To.ScanlineStrength, Alpha);
	Result.ColorSteps = FMath::Lerp(From.ColorSteps, To.ColorSteps, Alpha);
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
