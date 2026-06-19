// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDArtToneController.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPostProcessComponent;

USTRUCT(BlueprintType)
struct FSDArtToneSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixelate", meta = (ClampMin = "1.0"))
	float PixelCount = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixelate", meta = (ClampMin = "1.0"))
	float ScanlineCount = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixelate", meta = (ClampMin = "0.0"))
	float ScanlineStrength = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixelate", meta = (ClampMin = "1.0"))
	float ColorSteps = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
	float ExposureCompensation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color", meta = (ClampMin = "0.0"))
	float Saturation = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color", meta = (ClampMin = "0.0"))
	float Contrast = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens", meta = (ClampMin = "0.0"))
	float VignetteIntensity = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens", meta = (ClampMin = "0.0"))
	float FilmGrainIntensity = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens", meta = (ClampMin = "0.0"))
	float ChromaticAberrationIntensity = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloom", meta = (ClampMin = "0.0"))
	float BloomIntensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bloom", meta = (ClampMin = "0.0"))
	float BloomThreshold = 0.5f;
};

UCLASS(Blueprintable)
class SHOWDOWN_API ASDArtToneController : public AActor
{
	GENERATED_BODY()

public:
	ASDArtToneController();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Art Tone")
	void ApplyArtTone(const FSDArtToneSettings& NewSettings);

	UFUNCTION(BlueprintCallable, Category = "Art Tone")
	void BlendToArtTone(const FSDArtToneSettings& NewSettings, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Art Tone")
	void SetPixelCount(float NewPixelCount);

	UFUNCTION(BlueprintCallable, Category = "Art Tone")
	void SetScanlineCount(float NewScanlineCount);

	UFUNCTION(BlueprintCallable, Category = "Art Tone")
	void SetScanlineStrength(float NewScanlineStrength);

	UFUNCTION(BlueprintCallable, Category = "Art Tone")
	void SetColorSteps(float NewColorSteps);

	UFUNCTION(BlueprintPure, Category = "Art Tone")
	const FSDArtToneSettings& GetCurrentSettings() const { return CurrentSettings; }

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPostProcessComponent> PostProcessComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art Tone")
	TObjectPtr<UMaterialInterface> PixelatePostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art Tone")
	FSDArtToneSettings InitialSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art Tone")
	bool bApplyInitialSettingsOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art Tone")
	bool bUnbound = true;

private:
	void EnsurePixelateMaterialInstance();
	void ApplyPostProcessSettings(const FSDArtToneSettings& Settings);
	void ApplyPixelateParameters(const FSDArtToneSettings& Settings);
	static FSDArtToneSettings LerpSettings(const FSDArtToneSettings& From, const FSDArtToneSettings& To, float Alpha);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PixelateMID;

	FSDArtToneSettings CurrentSettings;
	FSDArtToneSettings BlendStartSettings;
	FSDArtToneSettings BlendTargetSettings;
	float BlendDuration = 0.0f;
	float BlendElapsedTime = 0.0f;
	bool bIsBlending = false;
};
