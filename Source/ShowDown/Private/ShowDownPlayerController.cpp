#include "ShowDownPlayerController.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Card.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Interaction/SDInteractable.h"
#include "PlayerPawn.h"
#include "ShowDownChatWidget.h"
#include "ShowDownGameModeBase.h"
#include "SlateOptMacros.h"
#include "Styling/CoreStyle.h"
#include "SupabaseSubsystem.h"
#include "TimerManager.h"
#include "Widgets/SLeafWidget.h"

class SSDCenterCrosshairWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSDCenterCrosshairWidget)
		: _CrosshairSize(18.0f)
		, _CrosshairThickness(2.0f)
		, _CrosshairColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.9f))
	{
	}
		SLATE_ARGUMENT(float, CrosshairSize)
		SLATE_ARGUMENT(float, CrosshairThickness)
		SLATE_ARGUMENT(FLinearColor, CrosshairColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		CrosshairSize = InArgs._CrosshairSize;
		CrosshairThickness = InArgs._CrosshairThickness;
		CrosshairColor = InArgs._CrosshairColor;
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
	{
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;
		const float HalfSize = CrosshairSize * 0.5f;
		const float HalfThickness = CrosshairThickness * 0.5f;
		const FSlateColor SlateColor(CrosshairColor);

		const FPaintGeometry HorizontalGeometry = AllottedGeometry.ToPaintGeometry(
			FVector2f(CrosshairSize, CrosshairThickness),
			FSlateLayoutTransform(FVector2f(Center.X - HalfSize, Center.Y - HalfThickness)));

		const FPaintGeometry VerticalGeometry = AllottedGeometry.ToPaintGeometry(
			FVector2f(CrosshairThickness, CrosshairSize),
			FSlateLayoutTransform(FVector2f(Center.X - HalfThickness, Center.Y - HalfSize)));

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			HorizontalGeometry,
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			SlateColor.GetColor(InWidgetStyle));

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			VerticalGeometry,
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			SlateColor.GetColor(InWidgetStyle));

		return LayerId + 1;
	}

private:
	float CrosshairSize = 18.0f;
	float CrosshairThickness = 2.0f;
	FLinearColor CrosshairColor = FLinearColor::White;
};

AShowDownPlayerController::AShowDownPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AShowDownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	InitializeFromPossessedPawn();
	CreateCenterCrosshairWidget();
	UpdateCenterCrosshairVisibility();
}

void AShowDownPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetHoveredCard(nullptr);
	RemoveCenterCrosshairWidget();
	Super::EndPlay(EndPlayReason);
}

void AShowDownPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	InitializeFromPossessedPawn();
}

void AShowDownPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!bHandleShowDownGameplayInput)
	{
		SetHoveredCard(nullptr);
		UpdateCenterCrosshairVisibility();
		return;
	}

	if (bChatOpen)
	{
		SetHoveredCard(nullptr);
		UpdateCenterCrosshairVisibility();
		if (WasInputKeyJustPressed(CloseChatKey))
		{
			CloseChat();
		}
		return;
	}

	if (WasInputKeyJustPressed(ToggleChatKey))
	{
		OpenChat();
		return;
	}

	if (bEnablePrimaryClickTrace && WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		HandlePrimaryClick();
	}

	if (bEnableLegacyKeyboardBetHotkeys)
	{
		HandleBettingHotkeys();
	}

	if (FixedCameraMouseLookTarget)
	{
		UpdateFixedCameraMouseLook();
	}
	else if (GetPawn() && bEnablePawnCameraMouseLook && (!bRequireRightMouseForPawnCameraLook || IsInputKeyDown(EKeys::RightMouseButton)))
	{
		float MouseDeltaX = 0.0f;
		float MouseDeltaY = 0.0f;
		GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
		if (!FMath::IsNearlyZero(MouseDeltaX) || !FMath::IsNearlyZero(MouseDeltaY))
		{
			ApplyPawnCameraInput(MouseDeltaX, -MouseDeltaY);
		}
	}

	UpdateHoveredCard();
	UpdateCenterCrosshairVisibility();
}

void AShowDownPlayerController::InitializeFromPossessedPawn()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	const APlayerPawn* ShowDownPawn = Cast<APlayerPawn>(GetPawn());
	if (!ShowDownPawn)
	{
		return;
	}

	if (!ChatWidgetClass)
	{
		ChatWidgetClass = ShowDownPawn->ChatWidgetClass;
	}

	LookSensitivity = ShowDownPawn->LookSensitivity;
	MinPitch = ShowDownPawn->MinPitch;
	MaxPitch = ShowDownPawn->MaxPitch;
	MinYaw = ShowDownPawn->MinYaw;
	MaxYaw = ShowDownPawn->MaxYaw;
	bEnableLegacyKeyboardBetHotkeys = ShowDownPawn->bEnableLegacyKeyboardBetHotkeys;
	ToggleChatKey = ShowDownPawn->ToggleChatKey;
	CloseChatKey = ShowDownPawn->CloseChatKey;
}

void AShowDownPlayerController::HandlePrimaryClick()
{
	FHitResult Hit;
	const bool bHasHit = TracePrimaryInteraction(Hit);

	if (bHasHit)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && bEnableInteractableTrace && HitActor->GetClass()->ImplementsInterface(USDInteractable::StaticClass()))
		{
			if (ISDInteractable::Execute_CanInteract(HitActor, this))
			{
				ISDInteractable::Execute_Interact(HitActor, this);
				return;
			}
		}

		HandCard = ResolveCardFromHit(Hit);
		if (HandCard && HandCard->IsCardSelectable())
		{
			SelectCard(HandCard);
			if (bSubmitCardsOnSingleClick)
			{
				SubmitSelectedCard(HandCard);
			}
			return;
		}
	}

	HandCard = bUseCenterAimCardFallback ? FindSelectableCardNearCenterAim() : nullptr;
	if (HandCard && HandCard->IsCardSelectable())
	{
		SelectCard(HandCard);
		if (bSubmitCardsOnSingleClick)
		{
			SubmitSelectedCard(HandCard);
		}
		return;
	}

	HandCard = nullptr;
}

void AShowDownPlayerController::TraceCardUnderCursor()
{
	HandCard = nullptr;

	FHitResult Hit;
	if (!TracePrimaryInteraction(Hit))
	{
		return;
	}

	HandCard = ResolveCardFromHit(Hit);
	if (HandCard && !HandCard->IsCardSelectable())
	{
		HandCard = nullptr;
	}
}

bool AShowDownPlayerController::TracePrimaryInteraction(FHitResult& OutHit) const
{
	if (bHandleShowDownGameplayInput && !bChatOpen)
	{
		return bUseCenterScreenTraceWhenCursorHidden && TraceFromScreenCenter(OutHit);
	}

	if (TraceUnderCursor(OutHit))
	{
		return true;
	}

	return bUseCenterScreenTraceWhenCursorHidden && TraceFromScreenCenter(OutHit);
}

bool AShowDownPlayerController::TraceUnderCursor(FHitResult& OutHit) const
{
	return GetHitResultUnderCursor(ECC_Visibility, false, OutHit);
}

bool AShowDownPlayerController::TraceFromScreenCenter(FHitResult& OutHit) const
{
	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return false;
	}

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;
	if (!DeprojectScreenPositionToWorld(
		static_cast<float>(ViewportSizeX) * 0.5f,
		static_cast<float>(ViewportSizeY) * 0.5f,
		WorldLocation,
		WorldDirection))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShowDownPrimaryInteraction), true);
	if (APawn* ControlledPawn = GetPawn())
	{
		QueryParams.AddIgnoredActor(ControlledPawn);
	}

	const FVector TraceEnd = WorldLocation + WorldDirection * CenterScreenTraceDistance;
	return World->LineTraceSingleByChannel(OutHit, WorldLocation, TraceEnd, ECC_Visibility, QueryParams);
}

ACard* AShowDownPlayerController::ResolveCardFromHit(const FHitResult& Hit) const
{
	if (ACard* HitCard = Cast<ACard>(Hit.GetActor()))
	{
		return HitCard;
	}

	if (const UPrimitiveComponent* HitComponent = Hit.GetComponent())
	{
		if (ACard* OwnerCard = Cast<ACard>(HitComponent->GetOwner()))
		{
			return OwnerCard;
		}
	}

	return nullptr;
}

ACard* AShowDownPlayerController::FindSelectableCardNearCenterAim() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return nullptr;
	}

	const FVector2D ScreenCenter(
		static_cast<float>(ViewportSizeX) * 0.5f,
		static_cast<float>(ViewportSizeY) * 0.5f);
	const float PickRadiusSq = FMath::Square(CenterAimCardPickRadiusPixels);

	ACard* BestCard = nullptr;
	float BestDistanceSq = PickRadiusSq;

	for (TActorIterator<ACard> It(World); It; ++It)
	{
		ACard* Card = *It;
		if (!IsValid(Card) || !Card->IsCardSelectable())
		{
			continue;
		}

		FVector2D CardScreenPosition = FVector2D::ZeroVector;
		if (!ProjectWorldLocationToScreen(Card->GetActorLocation(), CardScreenPosition, false))
		{
			continue;
		}

		const float DistanceSq = FVector2D::DistSquared(ScreenCenter, CardScreenPosition);
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestCard = Card;
		}
	}

	return BestCard;
}

ACard* AShowDownPlayerController::FindHoverPreviewCard() const
{
	if (!bEnableCardHoverPreview)
	{
		return nullptr;
	}

	FHitResult Hit;
	if (TracePrimaryInteraction(Hit))
	{
		ACard* HitCard = ResolveCardFromHit(Hit);
		if (IsValid(HitCard) && HitCard->IsCardSelectable())
		{
			return HitCard;
		}
	}

	return bUseCenterAimCardFallback ? FindSelectableCardNearCenterAim() : nullptr;
}

void AShowDownPlayerController::UpdateHoveredCard()
{
	SetHoveredCard(FindHoverPreviewCard());
}

void AShowDownPlayerController::SetHoveredCard(ACard* NewHoveredCard)
{
	if (!IsValid(NewHoveredCard) || !NewHoveredCard->IsCardSelectable())
	{
		NewHoveredCard = nullptr;
	}

	if (HoveredCard == NewHoveredCard)
	{
		return;
	}

	if (IsValid(HoveredCard))
	{
		HoveredCard->SetHovered(false);
	}

	HoveredCard = NewHoveredCard;

	if (IsValid(HoveredCard))
	{
		HoveredCard->SetHovered(true);
	}
}

void AShowDownPlayerController::SelectCard(ACard* SelectedCard)
{
	if (!IsValid(SelectedCard))
	{
		return;
	}

	if (CurrentSelectedCard && !IsValid(CurrentSelectedCard))
	{
		CurrentSelectedCard = nullptr;
	}

	if (CurrentSelectedCard == SelectedCard)
	{
		SubmitSelectedCard(SelectedCard);
		return;
	}

	if (CurrentSelectedCard)
	{
		CurrentSelectedCard->SelectCard(false);
	}

	SelectedCard->SelectCard(true);
	CurrentSelectedCard = SelectedCard;
}

void AShowDownPlayerController::SubmitSelectedCard(ACard* SelectedCard)
{
	if (!IsValid(SelectedCard))
	{
		return;
	}

	if (CurrentSelectedCard == SelectedCard)
	{
		CurrentSelectedCard->SelectCard(false);
		CurrentSelectedCard = nullptr;
	}

	if (HandCard == SelectedCard)
	{
		HandCard = nullptr;
	}

	if (HoveredCard == SelectedCard)
	{
		if (IsValid(HoveredCard))
		{
			HoveredCard->SetHovered(false);
		}
		HoveredCard = nullptr;
	}

	if (HasAuthority())
	{
		if (AShowDownGameModeBase* GameMode = ResolveGameMode())
		{
			GameMode->PlayerSelectedCardFromController(this, SelectedCard);
		}
		return;
	}

	ServerSubmitSelectedCard(SelectedCard);
}

void AShowDownPlayerController::ToggleChat()
{
	if (bChatOpen)
	{
		CloseChat();
	}
	else
	{
		OpenChat();
	}
}

void AShowDownPlayerController::OpenChat()
{
	SetHoveredCard(nullptr);
	EnsureChatWidget();
	if (!ChatWidget)
	{
		return;
	}

	bChatOpen = true;
	ChatWidget->SetVisibility(ESlateVisibility::Visible);
	ApplyChatInputMode(true);
	ChatWidget->FocusChatInput();

	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (bChatOpen && ChatWidget)
		{
			ChatWidget->FocusChatInput();
		}
	}));
}

void AShowDownPlayerController::CloseChat()
{
	bChatOpen = false;
	if (ChatWidget)
	{
		ChatWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	ApplyChatInputMode(false);
}

void AShowDownPlayerController::SubmitDialogueInput(const FString& Text)
{
	const FString TrimmedText = Text.TrimStartAndEnd();
	if (TrimmedText.IsEmpty())
	{
		return;
	}

	if (HasAuthority())
	{
		if (AShowDownGameModeBase* GameMode = ResolveGameMode())
		{
			GameMode->SubmitPlayerDialogueInputFromPlayer(TrimmedText, GetChatSenderName());
		}
		return;
	}

	ServerSubmitDialogueInput(TrimmedText, GetChatSenderName());
}

void AShowDownPlayerController::RequestPlayerCheck()
{
	SubmitPlayerBetAction(EShowDownBetAction::Check, 0);
}

void AShowDownPlayerController::RequestPlayerRaise()
{
	SubmitPlayerBetAction(EShowDownBetAction::Raise, 0);
}

void AShowDownPlayerController::RequestPlayerRaiseTo(int32 BulletCount)
{
	SubmitPlayerBetAction(EShowDownBetAction::Raise, BulletCount);
}

void AShowDownPlayerController::RequestPlayerFold()
{
	SubmitPlayerBetAction(EShowDownBetAction::Fold, 0);
}

void AShowDownPlayerController::SetFixedCameraMouseLook(
	ACameraActor* Camera,
	float Sensitivity,
	float MinPitchDegrees,
	float MaxPitchDegrees,
	float MinYawOffsetDegrees,
	float MaxYawOffsetDegrees,
	bool bInvertY)
{
	SetFixedCameraComponentMouseLook(
		Camera ? Camera->GetCameraComponent() : nullptr,
		Sensitivity,
		MinPitchDegrees,
		MaxPitchDegrees,
		MinYawOffsetDegrees,
		MaxYawOffsetDegrees,
		bInvertY);
}

void AShowDownPlayerController::SetFixedCameraComponentMouseLook(
	USceneComponent* CameraComponent,
	float Sensitivity,
	float MinPitchDegrees,
	float MaxPitchDegrees,
	float MinYawOffsetDegrees,
	float MaxYawOffsetDegrees,
	bool bInvertY)
{
	FixedCameraMouseLookTarget = CameraComponent;
	if (!FixedCameraMouseLookTarget)
	{
		return;
	}

	FixedCameraBaseRotation = FixedCameraMouseLookTarget->GetComponentRotation();
	FixedCameraLookSensitivity = Sensitivity;
	FixedCameraMinPitch = MinPitchDegrees;
	FixedCameraMaxPitch = MaxPitchDegrees;
	FixedCameraMinYawOffset = MinYawOffsetDegrees;
	FixedCameraMaxYawOffset = MaxYawOffsetDegrees;
	bFixedCameraInvertMouseY = bInvertY;
	UpdateCenterCrosshairVisibility();
}

void AShowDownPlayerController::ClearFixedCameraMouseLook()
{
	FixedCameraMouseLookTarget = nullptr;
	UpdateCenterCrosshairVisibility();
}

void AShowDownPlayerController::SubmitPlayerBetAction(EShowDownBetAction Action, int32 TargetBet)
{
	if (HasAuthority())
	{
		if (AShowDownGameModeBase* GameMode = ResolveGameMode())
		{
			GameMode->RequestPlayerBetActionFromController(this, Action, TargetBet);
		}
		return;
	}

	switch (Action)
	{
	case EShowDownBetAction::Check:
	case EShowDownBetAction::Call:
		ServerPlayerCheck();
		break;

	case EShowDownBetAction::Raise:
		if (TargetBet > 0)
		{
			ServerPlayerRaiseTo(TargetBet);
		}
		else
		{
			ServerPlayerRaise();
		}
		break;

	case EShowDownBetAction::Fold:
		ServerPlayerFold();
		break;

	default:
		break;
	}
}

void AShowDownPlayerController::ApplyPawnCameraInput(float YawInput, float PitchInput)
{
	const FRotator CurrentControlRotation = GetControlRotation();
	const float CurrentPitch = FRotator::NormalizeAxis(CurrentControlRotation.Pitch);
	const float CurrentYaw = FRotator::NormalizeAxis(CurrentControlRotation.Yaw);

	const float NewPitch = FMath::Clamp(CurrentPitch + PitchInput * LookSensitivity, MinPitch, MaxPitch);
	const float NewYaw = FMath::Clamp(CurrentYaw + YawInput * LookSensitivity, MinYaw, MaxYaw);

	SetControlRotation(FRotator(NewPitch, NewYaw, 0.0f));
}

void AShowDownPlayerController::UpdateFixedCameraMouseLook()
{
	if (!FixedCameraMouseLookTarget)
	{
		return;
	}

	float MouseDeltaX = 0.0f;
	float MouseDeltaY = 0.0f;
	GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
	if (FMath::IsNearlyZero(MouseDeltaX) && FMath::IsNearlyZero(MouseDeltaY))
	{
		return;
	}

	FRotator NewRotation = FixedCameraMouseLookTarget->GetComponentRotation();
	NewRotation.Yaw += MouseDeltaX * FixedCameraLookSensitivity;

	const float RelativeYaw = FRotator::NormalizeAxis(NewRotation.Yaw - FixedCameraBaseRotation.Yaw);
	const float ClampedRelativeYaw = FMath::Clamp(RelativeYaw, FixedCameraMinYawOffset, FixedCameraMaxYawOffset);
	NewRotation.Yaw = FixedCameraBaseRotation.Yaw + ClampedRelativeYaw;

	const float PitchInputSign = bFixedCameraInvertMouseY ? 1.0f : -1.0f;
	const float CurrentPitch = FRotator::NormalizeAxis(NewRotation.Pitch);
	NewRotation.Pitch = FMath::Clamp(
		CurrentPitch + MouseDeltaY * FixedCameraLookSensitivity * PitchInputSign,
		FixedCameraMinPitch,
		FixedCameraMaxPitch);
	NewRotation.Roll = 0.0f;

	FixedCameraMouseLookTarget->SetWorldRotation(NewRotation);
}

void AShowDownPlayerController::HandleBettingHotkeys()
{
	if (WasInputKeyJustPressed(EKeys::Q))
	{
		RequestPlayerCheck();
	}

	if (WasInputKeyJustPressed(EKeys::R))
	{
		RequestPlayerFold();
	}

	if (WasInputKeyJustPressed(EKeys::E))
	{
		RequestPlayerRaise();
	}

	if (WasInputKeyJustPressed(EKeys::One))
	{
		RequestPlayerRaiseTo(2);
	}

	if (WasInputKeyJustPressed(EKeys::Two))
	{
		RequestPlayerRaiseTo(3);
	}

	if (WasInputKeyJustPressed(EKeys::Three))
	{
		RequestPlayerRaiseTo(4);
	}

	if (WasInputKeyJustPressed(EKeys::Four))
	{
		RequestPlayerRaiseTo(5);
	}

	if (WasInputKeyJustPressed(EKeys::Five))
	{
		RequestPlayerRaiseTo(6);
	}
}

void AShowDownPlayerController::EnsureChatWidget()
{
	if (ChatWidget)
	{
		return;
	}

	if (!ChatWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ChatWidgetClass is not assigned on %s."), *GetName());
		return;
	}

	ChatWidget = CreateWidget<UShowDownChatWidget>(this, ChatWidgetClass);
	if (!ChatWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create chat widget on %s."), *GetName());
		return;
	}

	ChatWidget->SetOwningShowDownController(this);
	ChatWidget->AddToViewport();
	ChatWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void AShowDownPlayerController::ApplyChatInputMode(bool bOpen)
{
	if (bOpen && ChatWidget)
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(ChatWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	else if (FixedCameraMouseLookTarget)
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}
	else
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}
	UpdateCenterCrosshairVisibility();
}

void AShowDownPlayerController::CreateCenterCrosshairWidget()
{
	if (CenterCrosshairWidget.IsValid() || !IsLocalController())
	{
		return;
	}

	CenterCrosshairWidget = SNew(SSDCenterCrosshairWidget)
		.CrosshairSize(CenterCrosshairSize)
		.CrosshairThickness(CenterCrosshairThickness)
		.CrosshairColor(CenterCrosshairColor);

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(CenterCrosshairWidget.ToSharedRef(), 50);
	}
}

void AShowDownPlayerController::UpdateCenterCrosshairVisibility()
{
	if (!CenterCrosshairWidget.IsValid())
	{
		return;
	}

	const bool bShouldShow =
		bShowCenterCrosshair
		&& bHandleShowDownGameplayInput
		&& !bChatOpen
		&& !bShowMouseCursor;

	CenterCrosshairWidget->SetVisibility(bShouldShow ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
}

void AShowDownPlayerController::RemoveCenterCrosshairWidget()
{
	if (!CenterCrosshairWidget.IsValid())
	{
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(CenterCrosshairWidget.ToSharedRef());
	}
	CenterCrosshairWidget.Reset();
}

FString AShowDownPlayerController::GetChatSenderName() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const USupabaseSubsystem* SupabaseSubsystem = GameInstance->GetSubsystem<USupabaseSubsystem>())
		{
			const FString Nickname = SupabaseSubsystem->GetNickname().TrimStartAndEnd();
			if (!Nickname.IsEmpty())
			{
				return Nickname.Left(32);
			}
		}
	}

	const APlayerState* CurrentPlayerState = PlayerState;
	FString SenderName = CurrentPlayerState ? CurrentPlayerState->GetPlayerName() : TEXT("Player");
	SenderName = SenderName.TrimStartAndEnd();
	if (SenderName.IsEmpty() || SenderName.StartsWith(TEXT("DESKTOP-")))
	{
		return TEXT("Player");
	}

	return SenderName.Left(32);
}

AShowDownGameModeBase* AShowDownPlayerController::ResolveGameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<AShowDownGameModeBase>() : nullptr;
}

void AShowDownPlayerController::ServerSubmitSelectedCard_Implementation(ACard* SelectedCard)
{
	if (SelectedCard)
	{
		if (AShowDownGameModeBase* GameMode = ResolveGameMode())
		{
			GameMode->PlayerSelectedCardFromController(this, SelectedCard);
		}
	}
}

void AShowDownPlayerController::ServerSubmitDialogueInput_Implementation(const FString& Text, const FString& SenderName)
{
	const FString TrimmedText = Text.TrimStartAndEnd();
	if (!TrimmedText.IsEmpty())
	{
		if (AShowDownGameModeBase* GameMode = ResolveGameMode())
		{
			GameMode->SubmitPlayerDialogueInputFromPlayer(TrimmedText, SenderName);
		}
	}
}

void AShowDownPlayerController::ServerPlayerCheck_Implementation()
{
	SubmitPlayerBetAction(EShowDownBetAction::Check, 0);
}

void AShowDownPlayerController::ServerPlayerRaise_Implementation()
{
	SubmitPlayerBetAction(EShowDownBetAction::Raise, 0);
}

void AShowDownPlayerController::ServerPlayerRaiseTo_Implementation(int32 BulletCount)
{
	SubmitPlayerBetAction(EShowDownBetAction::Raise, BulletCount);
}

void AShowDownPlayerController::ServerPlayerFold_Implementation()
{
	SubmitPlayerBetAction(EShowDownBetAction::Fold, 0);
}

void AShowDownPlayerController::ClientShowStatusMessage_Implementation(const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("ShowDown status: %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, Message);
	}
}
