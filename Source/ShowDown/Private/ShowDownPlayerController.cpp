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
#include "SDPlayerState.h"
#include "SDMultiplayerTable.h"
#include "ShowDownChatWidget.h"
#include "ShowDownEosSubsystem.h"
#include "ShowDownGameModeBase.h"
#include "ShowDownLeaveConfirmWidget.h"
#include "UObject/ConstructorHelpers.h"
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

namespace
{
	int32 GetSeatIndexFromPlayerSlot(EShowDownPlayerSlot Slot)
	{
		switch (Slot)
		{
		case EShowDownPlayerSlot::Player1: return 0;
		case EShowDownPlayerSlot::Player2: return 1;
		case EShowDownPlayerSlot::Player3: return 2;
		case EShowDownPlayerSlot::Player4: return 3;
		default: return INDEX_NONE;
		}
	}

	bool IsMultiplayerGameMap(const UWorld* World)
	{
		return World && World->GetMapName().Contains(TEXT("L_MultiplayerGame"));
	}
}

AShowDownPlayerController::AShowDownPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	// Keep multiplayer chat functional even when the level pawn Blueprint has
	// not supplied an override. This also makes the widget available early so it
	// can subscribe to replicated GameState chat broadcasts before the user
	// presses the chat key.
	static ConstructorHelpers::FClassFinder<UShowDownChatWidget> ChatWidgetBlueprint(
		TEXT("/Game/UI/WBP_Chat"));
	if (ChatWidgetBlueprint.Succeeded())
	{
		ChatWidgetClass = ChatWidgetBlueprint.Class;
	}
	else
	{
		ChatWidgetClass = UShowDownChatWidget::StaticClass();
	}
}

void AShowDownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	InitializeFromPossessedPawn();
	SubmitLocalMultiplayerDisplayName();
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
	SubmitLocalMultiplayerDisplayName();
}

void AShowDownPlayerController::ClientEnterMultiplayerGameplay_Implementation()
{
	if (UShowDownEosSubsystem* EosSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UShowDownEosSubsystem>()
		: nullptr)
	{
		EosSubsystem->MarkEnteredMultiplayerGame();
	}

	RemoveCenterCrosshairWidget();
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveAllViewportWidgets();
	}
	ChatWidget = nullptr;
	LeaveConfirmWidget = nullptr;
	bChatOpen = false;

	bHandleShowDownGameplayInput = true;
	// Multiplayer follows the same first-person interaction model as single
	// player: raw mouse movement controls the view and the centre reticle is
	// used for card/interactable tracing. UI temporarily reveals the cursor.
	bShowCenterCrosshair = true;
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	RestoreMultiplayerGameplayInput();
	EnsureChatWidget();
	CreateCenterCrosshairWidget();
	PawnCameraBaseRotation = GetControlRotation();
	bHasPawnCameraBaseRotation = true;
	UpdateCenterCrosshairVisibility();
}

void AShowDownPlayerController::ClientUseMultiplayerSeatCamera_Implementation(int32 SeatIndex)
{
	// A client can receive this RPC while its seamless level transition is still
	// loading. Keep the seat number and retry from PlayerTick once the target
	// map's CameraActors actually exist locally.
	PendingMultiplayerSeatIndex = SeatIndex;
	bPendingMultiplayerSeatCamera = true;
	bHandleShowDownGameplayInput = true;
	bShowCenterCrosshair = true;
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	RestoreMultiplayerGameplayInput();

	if (TryApplyPendingMultiplayerSeatCamera())
	{
		return;
	}

	// Until the game map finishes loading, retain a safe local pawn view.
	ClearFixedCameraMouseLook();
	if (APawn* ControlledPawn = GetPawn())
	{
		SetViewTarget(ControlledPawn);
	}
	UpdateCenterCrosshairVisibility();
}

bool AShowDownPlayerController::TryApplyPendingMultiplayerSeatCamera()
{
	if (!bPendingMultiplayerSeatCamera || PendingMultiplayerSeatIndex == INDEX_NONE || !GetWorld())
	{
		return false;
	}

	const FName SeatCameraTag(*FString::Printf(TEXT("MP_SeatCamera_%d"), PendingMultiplayerSeatIndex + 1));
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		ACameraActor* SeatCamera = *It;
		if (!SeatCamera || !SeatCamera->ActorHasTag(SeatCameraTag))
		{
			continue;
		}

		SetViewTarget(SeatCamera);
		SetFixedCameraMouseLook(SeatCamera, 0.08f, -35.0f, 35.0f, -45.0f, 45.0f, true);
		EnsureChatWidget();
		bPendingMultiplayerSeatCamera = false;
		bHandleShowDownGameplayInput = true;
		bShowCenterCrosshair = true;
		RestoreMultiplayerGameplayInput();
		UpdateCenterCrosshairVisibility();
		UE_LOG(LogTemp, Log, TEXT("멀티플레이 좌석 카메라 적용: %s"), *SeatCameraTag.ToString());
		return true;
	}

	// Some older copies of the level do not contain the named CameraActors (or
	// do not preserve their actor tags after a map migration). Do not leave the
	// player at a PlayerStart in that case: create the same four table views
	// locally from the authoritative replicated table position.
	return IsMultiplayerGameMap(GetWorld())
		&& UseFallbackMultiplayerSeatCamera(PendingMultiplayerSeatIndex);
}

bool AShowDownPlayerController::UseFallbackMultiplayerSeatCamera(int32 SeatIndex)
{
	if (!IsLocalController() || !GetWorld() || SeatIndex < 0 || SeatIndex > 3)
	{
		return false;
	}

	static const FVector SeatOffsets[] = {
		FVector(0.0f, -850.0f, 220.0f),
		FVector(0.0f, 850.0f, 220.0f),
		FVector(-1200.0f, 0.0f, 220.0f),
		FVector(1200.0f, 0.0f, 220.0f)
	};
	static const float SeatYaws[] = { 90.0f, -90.0f, 0.0f, 180.0f };

	FVector TableCenter(0.0f, 0.0f, 25.0f);
	for (TActorIterator<ASDMultiplayerTable> It(GetWorld()); It; ++It)
	{
		if (const ASDMultiplayerTable* Table = *It)
		{
			TableCenter = Table->GetActorLocation();
			break;
		}
	}

	const FTransform CameraTransform(
		FRotator(-12.0f, SeatYaws[SeatIndex], 0.0f),
		TableCenter + SeatOffsets[SeatIndex]);
	if (!IsValid(LocalFallbackSeatCamera))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		LocalFallbackSeatCamera = GetWorld()->SpawnActor<ACameraActor>(
			ACameraActor::StaticClass(), CameraTransform, SpawnParams);
	}
	else
	{
		LocalFallbackSeatCamera->SetActorTransform(CameraTransform);
	}

	if (!IsValid(LocalFallbackSeatCamera))
	{
		return false;
	}

	SetViewTarget(LocalFallbackSeatCamera);
	SetFixedCameraMouseLook(LocalFallbackSeatCamera, 0.12f, -30.0f, 25.0f, -55.0f, 55.0f, true);
	EnsureChatWidget();
	bPendingMultiplayerSeatCamera = false;
	bHandleShowDownGameplayInput = true;
	bShowCenterCrosshair = true;
	RestoreMultiplayerGameplayInput();
	UpdateCenterCrosshairVisibility();
	UE_LOG(LogTemp, Warning, TEXT("Authored seat camera missing; using fallback multiplayer seat camera %d."), SeatIndex + 1);
	return true;
}

void AShowDownPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Do not rely solely on the server RPC: with seamless travel a remote
	// client can finish loading after that RPC was delivered. Its replicated
	// PlayerState slot is the durable source of truth for its camera seat.
	if (!FixedCameraMouseLookTarget && IsMultiplayerGameMap(GetWorld()))
	{
		if (const ASDPlayerState* ShowDownPlayerState = GetPlayerState<ASDPlayerState>())
		{
			const int32 SeatIndex = GetSeatIndexFromPlayerSlot(ShowDownPlayerState->ShowDownSlot);
			if (SeatIndex != INDEX_NONE)
			{
				PendingMultiplayerSeatIndex = SeatIndex;
				bPendingMultiplayerSeatCamera = true;
			}
		}
	}
	TryApplyPendingMultiplayerSeatCamera();

	if (!bHandleShowDownGameplayInput)
	{
		SetHoveredCard(nullptr);
		UpdateCenterCrosshairVisibility();
		return;
	}

	if (LeaveConfirmWidget && LeaveConfirmWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		if (WasInputKeyJustPressed(LeaveMatchKey))
		{
			CancelLeaveMultiplayerMatch();
		}
		SetHoveredCard(nullptr);
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

	if (WasInputKeyJustPressed(LeaveMatchKey))
	{
		RequestLeaveMultiplayerMatch();
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

	// Every multiplayer pawn starts facing the shared table. Keep camera limits
	// relative to that seat direction instead of clamping to world-space yaw.
	PawnCameraBaseRotation = GetControlRotation();
	bHasPawnCameraBaseRotation = true;
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
	if (bShowMouseCursor && TraceUnderCursor(OutHit))
	{
		return true;
	}

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

void AShowDownPlayerController::RequestLeaveMultiplayerMatch()
{
	EnsureLeaveConfirmWidget();
	if (!LeaveConfirmWidget)
	{
		return;
	}

	if (LeaveConfirmWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		CancelLeaveMultiplayerMatch();
		return;
	}

	LeaveConfirmWidget->SetVisibility(ESlateVisibility::Visible);
	bShowMouseCursor = true;
	SetHoveredCard(nullptr);
	SetIgnoreLookInput(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(LeaveConfirmWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AShowDownPlayerController::ConfirmLeaveMultiplayerMatch()
{
	if (UShowDownEosSubsystem* EosSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UShowDownEosSubsystem>()
		: nullptr)
	{
		if (LeaveConfirmWidget)
		{
			LeaveConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		bHandleShowDownGameplayInput = false;
		SetHoveredCard(nullptr);
		EosSubsystem->LeaveLobby(FName(TEXT("L_Hub")));
	}
}

void AShowDownPlayerController::CancelLeaveMultiplayerMatch()
{
	if (LeaveConfirmWidget)
	{
		LeaveConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	RestoreMultiplayerGameplayInput();
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
	if (!bHasPawnCameraBaseRotation)
	{
		PawnCameraBaseRotation = CurrentControlRotation;
		bHasPawnCameraBaseRotation = true;
	}

	const float CurrentPitch = FRotator::NormalizeAxis(CurrentControlRotation.Pitch);
	const float CurrentYawOffset = FRotator::NormalizeAxis(CurrentControlRotation.Yaw - PawnCameraBaseRotation.Yaw);

	const float NewPitch = FMath::Clamp(
		CurrentPitch + PitchInput * LookSensitivity,
		PawnCameraBaseRotation.Pitch + MinPitch,
		PawnCameraBaseRotation.Pitch + MaxPitch);
	const float NewYawOffset = FMath::Clamp(CurrentYawOffset + YawInput * LookSensitivity, MinYaw, MaxYaw);
	const float NewYaw = PawnCameraBaseRotation.Yaw + NewYawOffset;

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

void AShowDownPlayerController::EnsureLeaveConfirmWidget()
{
	if (LeaveConfirmWidget)
	{
		return;
	}

	LeaveConfirmWidget = CreateWidget<UShowDownLeaveConfirmWidget>(this, UShowDownLeaveConfirmWidget::StaticClass());
	if (!LeaveConfirmWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("멀티플레이 퇴장 확인창을 만들지 못했습니다."));
		return;
	}

	LeaveConfirmWidget->SetOwningShowDownController(this);
	LeaveConfirmWidget->AddToViewport(100);
	LeaveConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void AShowDownPlayerController::RestoreMultiplayerGameplayInput()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetIgnoreLookInput(false);

	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);
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

void AShowDownPlayerController::SubmitLocalMultiplayerDisplayName()
{
	if (!IsLocalController())
	{
		return;
	}

	const USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USupabaseSubsystem>()
		: nullptr;
	if (!SupabaseSubsystem)
	{
		return;
	}

	const FString Nickname = SupabaseSubsystem->GetNickname().TrimStartAndEnd();
	if (!Nickname.IsEmpty())
	{
		ServerSetMultiplayerDisplayName(Nickname);
	}
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

void AShowDownPlayerController::ServerSetMultiplayerDisplayName_Implementation(const FString& DisplayName)
{
	const FString SanitizedName = DisplayName.TrimStartAndEnd().Left(32);
	if (SanitizedName.IsEmpty())
	{
		return;
	}

	if (APlayerState* CurrentPlayerState = PlayerState)
	{
		CurrentPlayerState->SetPlayerName(SanitizedName);
		if (AShowDownGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AShowDownGameModeBase>() : nullptr)
		{
			GameMode->RefreshMultiplayerLobbyPlayers();
		}
	}
}

void AShowDownPlayerController::ClientShowStatusMessage_Implementation(const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("ShowDown status: %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green, Message);
	}
}
