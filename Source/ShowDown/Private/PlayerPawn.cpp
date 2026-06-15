#include "PlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Card.h"
#include "InputCoreTypes.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "ShowDownGameModeBase.h"

APlayerPawn::APlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// 루트 메시 컴포넌트 생성
	rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(rootComp);

	// 카메라 컴포넌트 생성
	cameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	cameraComp->SetupAttachment(rootComp);
	cameraComp->bUsePawnControlRotation = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;

	// 손패 카드 슬롯
	PlayerHandCard = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerHandRoot"));
	PlayerHandCard->SetupAttachment(rootComp);

	// 머리 위 카드 슬롯
	PlayerHeadCard = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerHeadCardSlot"));
	PlayerHeadCard->SetupAttachment(rootComp);
}

void APlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	// GameModeBase 할당
	ModeBase = Cast<AShowDownGameModeBase>(
		UGameplayStatics::GetGameMode(GetWorld())
	);

	AddInputMappingContext();
}

void APlayerPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AddInputMappingContext();
	bHasPreviousMousePosition = false;
}

void APlayerPawn::AddInputMappingContext()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;

		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (imc_SD)
				{
					Subsystem->AddMappingContext(imc_SD, 0);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("imc_SD is not assigned on %s."), *GetName());
				}
			}
		}
	}
}

void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		bHasPreviousMousePosition = false;
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		bHasPreviousMousePosition = false;
		return;
	}

	if (!bHasPreviousMousePosition)
	{
		PreviousMouseX = MouseX;
		PreviousMouseY = MouseY;
		bHasPreviousMousePosition = true;
		return;
	}

	const float DeltaX = MouseX - PreviousMouseX;
	const float DeltaY = MouseY - PreviousMouseY;

	PreviousMouseX = MouseX;
	PreviousMouseY = MouseY;

	if (!FMath::IsNearlyZero(DeltaX) || !FMath::IsNearlyZero(DeltaY))
	{
		ApplyCameraInput(DeltaX, -DeltaY);
	}
	
	HandleBettingHotkeys();
}

void APlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto PlayerInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!PlayerInput)
	{
		UE_LOG(LogTemp, Error, TEXT("EnhancedInputComponent is missing."));
		return;
	}
	
	if (ia_Select)
	{
		PlayerInput->BindAction(ia_Select, ETriggerEvent::Started, this, &APlayerPawn::InputSelect);
	}

	if (ia_LookUp)
	{
		PlayerInput->BindAction(ia_LookUp, ETriggerEvent::Triggered, this, &APlayerPawn::LookUp);
	}

	if (ia_Turn)
	{
		PlayerInput->BindAction(ia_Turn, ETriggerEvent::Triggered, this, &APlayerPawn::Turn);
	}
}

void APlayerPawn::InputSelect(const FInputActionValue& InputValue)
{
	TraceCardUnderCursor();

	if (HandCard)
	{
		SelectCard(HandCard);
	}
}

void APlayerPawn::TraceCardUnderCursor()
{
	HandCard = nullptr;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	FHitResult Hit;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	HandCard = Cast<ACard>(Hit.GetActor());
	if (HandCard && !HandCard->IsCardSelectable())
	{
		HandCard = nullptr;
	}
}

void APlayerPawn::SelectCard(ACard* SelectedCard)
{
	if (!SelectedCard)
	{
		return;
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

void APlayerPawn::SubmitSelectedCard(ACard* SelectedCard)
{
	if (!SelectedCard)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Submit selected card: %s"), *SelectedCard->GetName());

	if (ModeBase)
	{
		ModeBase->PlayerSelectedCard(SelectedCard);
	}
}

// 상하 회전 입력에 따른 콜백 함수 구현
void APlayerPawn::LookUp(const FInputActionValue& inputValue)
{
	float value = inputValue.Get<float>();
	ApplyCameraInput(0.0f, value);
}

// 좌우 회전 입력에 따른 콜백 함수 구현
void APlayerPawn::Turn(const FInputActionValue& inputValue)
{
	float value = inputValue.Get<float>();
	ApplyCameraInput(value, 0.0f);
}

void APlayerPawn::ApplyCameraInput(float YawInput, float PitchInput)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	const FRotator ControlRotation = PC->GetControlRotation();
	const float CurrentPitch = FRotator::NormalizeAxis(ControlRotation.Pitch);
	const float CurrentYaw = FRotator::NormalizeAxis(ControlRotation.Yaw);

	const float NewPitch = FMath::Clamp(CurrentPitch + PitchInput * LookSensitivity, MinPitch, MaxPitch);
	const float NewYaw = FMath::Clamp(CurrentYaw + YawInput * LookSensitivity, MinYaw, MaxYaw);

	PC->SetControlRotation(FRotator(NewPitch, NewYaw, 0.0f));
}

void APlayerPawn::HandleBettingHotkeys()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !ModeBase)
	{
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::Q))
	{
		ModeBase->PlayerCheck();
	}

	if (PC->WasInputKeyJustPressed(EKeys::R))
	{
		ModeBase->PlayerFold();
	}

	if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		ModeBase->PlayerRaise();
	}

	if (PC->WasInputKeyJustPressed(EKeys::One))
	{
		ModeBase->PlayerRaiseTo(2);
	}

	if (PC->WasInputKeyJustPressed(EKeys::Two))
	{
		ModeBase->PlayerRaiseTo(3);
	}

	if (PC->WasInputKeyJustPressed(EKeys::Three))
	{
		ModeBase->PlayerRaiseTo(4);
	}

	if (PC->WasInputKeyJustPressed(EKeys::Four))
	{
		ModeBase->PlayerRaiseTo(5);
	}

	if (PC->WasInputKeyJustPressed(EKeys::Five))
	{
		ModeBase->PlayerRaiseTo(6);
	}
}
