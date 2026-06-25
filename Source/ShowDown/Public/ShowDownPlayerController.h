#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "ShowDownTypes.h"
#include "ShowDownPlayerController.generated.h"

class ACard;
class ACameraActor;
class AShowDownGameModeBase;
class SWidget;
class USceneComponent;
class UShowDownChatWidget;
class UShowDownLeaveConfirmWidget;
class UShowDownMultiRankWidget;

UCLASS()
class SHOWDOWN_API AShowDownPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShowDownPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Input")
	void HandlePrimaryClick();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Card")
	void SubmitSelectedCard(ACard* SelectedCard);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Chat")
	void ToggleChat();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Chat")
	void OpenChat();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Chat")
	void CloseChat();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Chat")
	void SubmitDialogueInput(const FString& Text);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	void RequestPlayerCheck();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	void RequestPlayerRaise();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	void RequestPlayerRaiseTo(int32 BulletCount);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Betting")
	void RequestPlayerFold();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Multiplayer")
	void RequestLeaveMultiplayerMatch();

	void ConfirmLeaveMultiplayerMatch();
	void CancelLeaveMultiplayerMatch();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Camera")
	void SetFixedCameraMouseLook(
		ACameraActor* Camera,
		float Sensitivity,
		float MinPitchDegrees,
		float MaxPitchDegrees,
		float MinYawOffsetDegrees,
		float MaxYawOffsetDegrees,
		bool bInvertY);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Camera")
	void ClearFixedCameraMouseLook();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Camera")
	void SetFixedCameraComponentMouseLook(
		USceneComponent* CameraComponent,
		float Sensitivity,
		float MinPitchDegrees,
		float MaxPitchDegrees,
		float MinYawOffsetDegrees,
		float MaxYawOffsetDegrees,
		bool bInvertY);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShowDown|Input")
	bool HandlesShowDownGameplayInput() const { return bHandleShowDownGameplayInput; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input")
	bool bHandleShowDownGameplayInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input")
	bool bEnableLegacyKeyboardBetHotkeys = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input")
	bool bEnablePrimaryClickTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input")
	bool bEnableInteractableTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input")
	bool bSubmitCardsOnSingleClick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input")
	bool bUseCenterScreenTraceWhenCursorHidden = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input", meta = (ClampMin = "0.0"))
	float CenterScreenTraceDistance = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input")
	bool bUseCenterAimCardFallback = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input")
	bool bEnableCardHoverPreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input", meta = (ClampMin = "1.0"))
	float CenterAimCardPickRadiusPixels = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input|Crosshair")
	bool bShowCenterCrosshair = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input|Crosshair", meta = (ClampMin = "1.0"))
	float CenterCrosshairSize = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input|Crosshair", meta = (ClampMin = "1.0"))
	float CenterCrosshairThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Input|Crosshair")
	FLinearColor CenterCrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Camera")
	bool bEnablePawnCameraMouseLook = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Camera")
	bool bRequireRightMouseForPawnCameraLook = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Camera")
	float LookSensitivity = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Camera")
	float MinPitch = -35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Camera")
	float MaxPitch = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Camera")
	float MinYaw = -45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Camera")
	float MaxYaw = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ShowDown|Chat")
	TSubclassOf<UShowDownChatWidget> ChatWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ShowDown|Multiplayer")
	TSubclassOf<UShowDownMultiRankWidget> MultiplayerRankWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Chat")
	FKey ToggleChatKey = EKeys::T;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Chat")
	FKey CloseChatKey = EKeys::Escape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShowDown|Multiplayer")
	FKey LeaveMatchKey = EKeys::Escape;

	UFUNCTION(Server, Reliable)
	void ServerSubmitSelectedCard(ACard* SelectedCard);

	UFUNCTION(Server, Reliable)
	void ServerSubmitDialogueInput(const FString& Text, const FString& SenderName);

	UFUNCTION(Server, Reliable)
	void ServerPlayerCheck();

	UFUNCTION(Server, Reliable)
	void ServerPlayerRaise();

	UFUNCTION(Server, Reliable)
	void ServerPlayerRaiseTo(int32 BulletCount);

	UFUNCTION(Server, Reliable)
	void ServerPlayerFold();

	UFUNCTION(Server, Reliable)
	void ServerSetMultiplayerDisplayName(const FString& DisplayName);

	UFUNCTION(Server, Reliable)
	void ServerRequestMultiplayerRestart();

	UFUNCTION(Client, Reliable)
	void ClientShowStatusMessage(const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientShowMultiplayerRank(const TArray<FString>& PlayerNames);

	// Restores gameplay input after travelling from the UI-only multiplayer lobby.
	UFUNCTION(Client, Reliable)
	void ClientEnterMultiplayerGameplay();

	// Selects one of the level-placed multiplayer seat cameras by its zero-based index.
	UFUNCTION(Client, Reliable)
	void ClientUseMultiplayerSeatCamera(int32 SeatIndex);

	UFUNCTION(Client, Reliable)
	void ClientLeaveMultiplayerRoomToHub();

private:
	void InitializeFromPossessedPawn();
	void TraceCardUnderCursor();
	bool TracePrimaryInteraction(FHitResult& OutHit) const;
	bool TraceUnderCursor(FHitResult& OutHit) const;
	bool TraceFromScreenCenter(FHitResult& OutHit) const;
	ACard* ResolveCardFromHit(const FHitResult& Hit) const;
	ACard* FindSelectableCardNearCenterAim() const;
	ACard* FindHoverPreviewCard() const;
	void UpdateHoveredCard();
	void SetHoveredCard(ACard* NewHoveredCard);
	void SelectCard(ACard* SelectedCard);
	void SubmitPlayerBetAction(EShowDownBetAction Action, int32 TargetBet);
	void ApplyPawnCameraInput(float YawInput, float PitchInput);
	void UpdateFixedCameraMouseLook();
	bool TryApplyPendingMultiplayerSeatCamera();
	bool UseFallbackMultiplayerSeatCamera(int32 SeatIndex);
	void HandleBettingHotkeys();
	void EnsureChatWidget();
	void EnsureLeaveConfirmWidget();
	void RestoreMultiplayerGameplayInput();
	void ApplyChatInputMode(bool bOpen);
	void CreateCenterCrosshairWidget();
	void UpdateCenterCrosshairVisibility();
	void RemoveCenterCrosshairWidget();
	void SubmitLocalMultiplayerDisplayName();
	FString GetChatSenderName() const;
	UFUNCTION()
	void HandleMultiRankRestartRequested();
	UFUNCTION()
	void HandleMultiRankMainMenuRequested();
	AShowDownGameModeBase* ResolveGameMode() const;

	UPROPERTY()
	ACard* HandCard = nullptr;

	UPROPERTY()
	ACard* CurrentSelectedCard = nullptr;

	UPROPERTY()
	ACard* HoveredCard = nullptr;

	UPROPERTY()
	UShowDownChatWidget* ChatWidget = nullptr;

	UPROPERTY()
	UShowDownLeaveConfirmWidget* LeaveConfirmWidget = nullptr;

	UPROPERTY()
	UShowDownMultiRankWidget* MultiplayerRankWidget = nullptr;

	UPROPERTY()
	TObjectPtr<USceneComponent> FixedCameraMouseLookTarget = nullptr;

	// Created only on a local client when the map does not contain an authored
	// MP_SeatCamera_* actor. It is deliberately non-replicated: every player
	// must keep an independent, slot-specific view of the shared table.
	TObjectPtr<ACameraActor> LocalFallbackSeatCamera = nullptr;

	TSharedPtr<SWidget> CenterCrosshairWidget;

	bool bChatOpen = false;
	bool bPendingMultiplayerSeatCamera = false;
	int32 PendingMultiplayerSeatIndex = INDEX_NONE;
	FRotator PawnCameraBaseRotation = FRotator::ZeroRotator;
	bool bHasPawnCameraBaseRotation = false;
	FRotator FixedCameraBaseRotation = FRotator::ZeroRotator;
	float FixedCameraLookSensitivity = 0.2f;
	float FixedCameraMinPitch = -35.0f;
	float FixedCameraMaxPitch = 35.0f;
	float FixedCameraMinYawOffset = -45.0f;
	float FixedCameraMaxYawOffset = 45.0f;
	bool bFixedCameraInvertMouseY = true;
};
