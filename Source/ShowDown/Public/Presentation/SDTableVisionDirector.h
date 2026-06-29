#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDTableVisionDirector.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UPostProcessComponent;
class USceneComponent;
class USpotLightComponent;

USTRUCT(BlueprintType)
struct FSDTableVisionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "0.0"))
	float VisionRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "0.0"))
	float VisionFeather = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DarknessStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (ClampMin = "0.0"))
	float SpotLightIntensity = 6500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (ClampMin = "0.0"))
	float SpotLightAttenuationRadius = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (ClampMin = "0.0"))
	float PointLightIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (ClampMin = "0.0"))
	float PointLightAttenuationRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process")
	float ExposureCompensation = -1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process", meta = (ClampMin = "0.0"))
	float VignetteIntensity = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process", meta = (ClampMin = "0.0"))
	float BloomIntensity = 0.12f;
};

UCLASS(Blueprintable)
class SHOWDOWN_API ASDTableVisionDirector : public AActor
{
	GENERATED_BODY()

public:
	ASDTableVisionDirector();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Table Vision")
	void ApplyCollapsedVision();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Table Vision")
	void ApplyRevealedVision();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Table Vision")
	void SetRevealAlpha(float Alpha);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Table Vision")
	void PlayRevealPulse(float ExpandTime = -1.0f, float HoldTime = -1.0f, float CollapseTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Table Vision")
	void RevealSurroundings(float ExpandTime = -1.0f, float HoldTime = -1.0f, float CollapseTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Table Vision")
	void StopRevealPulse(bool bSnapToCollapsed = true);

	UFUNCTION(BlueprintPure, Category = "ShowDown|Table Vision")
	float GetRevealAlpha() const { return CurrentRevealAlpha; }

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPostProcessComponent> PostProcessComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpotLightComponent> TableSpotLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> TableFillLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision")
	TObjectPtr<AActor> VisionCenterActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision")
	FSDTableVisionState CollapsedVision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision")
	FSDTableVisionState RevealedVision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Light", meta = (ClampMin = "0.0"))
	float OverheadLightHeight = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Light")
	FLinearColor LightColor = FLinearColor(1.0f, 0.82f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Light", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float SpotLightInnerConeAngle = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Light", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float SpotLightOuterConeAngle = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Light")
	bool bUsePointFillLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Post Process")
	TObjectPtr<UMaterialInterface> DarknessPostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Post Process")
	bool bEnableDarknessPostProcess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Post Process")
	bool bAutoLoadDefaultDarknessMaterial = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Post Process")
	bool bUnboundPostProcess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Post Process")
	bool bPreviewPostProcessInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Post Process")
	float PostProcessPriority = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Animation", meta = (ClampMin = "0.0"))
	float RevealExpandTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Animation", meta = (ClampMin = "0.0"))
	float RevealHoldTime = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Animation", meta = (ClampMin = "0.0"))
	float RevealCollapseTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Animation", meta = (ClampMin = "0.01"))
	float RevealEaseExponent = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Animation")
	bool bApplyInitialStateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Animation")
	bool bPlayRevealOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Animation")
	bool bTrackVisionCenterEveryTick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Table Vision|Editor Preview", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EditorPreviewRevealAlpha = 0.0f;

private:
	void EnsureDarknessMaterialInstance();
	void ApplyCurrentState();
	void ApplyLightState();
	void ApplyPostProcessState();
	void ApplyDarknessMaterialState();
	void UpdateLightTransform();
	void UpdateTickState();
	void UpdateRevealPulse(float DeltaSeconds);
	FVector GetVisionCenterWorldLocation() const;
	bool ShouldApplyPostProcessInCurrentWorld() const;
	float EaseUnitAlpha(float Alpha) const;
	static FSDTableVisionState LerpVisionState(const FSDTableVisionState& From, const FSDTableVisionState& To, float Alpha);
	static bool IsMatchingBlendable(const UObject* BlendableObject, const UMaterialInterface* Material, const UMaterialInstanceDynamic* MID);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DarknessMID;

	FSDTableVisionState CurrentState;
	float CurrentRevealAlpha = 0.0f;
	float ActiveExpandTime = 0.0f;
	float ActiveHoldTime = 0.0f;
	float ActiveCollapseTime = 0.0f;
	float RevealPulseElapsed = 0.0f;
	bool bRevealPulseActive = false;
};
