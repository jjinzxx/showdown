#include "ShowDownHubFlowManager.h"

#include "Camera/CameraActor.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ShowDownGameModeBase.h"
#include "ShowDownLoginWidget.h"
#include "ShowDownMainMenuWidget.h"
#include "ShowDownShopWidget.h"
#include "SupabaseSubsystem.h"

AShowDownHubFlowManager::AShowDownHubFlowManager()
{
	PrimaryActorTick.bCanEverTick = false;
	ShopWidgetClass = UShowDownShopWidget::StaticClass();
}

void AShowDownHubFlowManager::BeginPlay()
{
	Super::BeginPlay();

	// GameInstanceSubsystem survives level travel, so returning players can skip login.
	bool bHasSession = false;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (const USupabaseSubsystem* SupabaseSubsystem = GameInstance->GetSubsystem<USupabaseSubsystem>())
		{
			bHasSession = !SupabaseSubsystem->GetAccessToken().IsEmpty();
		}
	}

	// 시작 순간엔 폰 스폰 카메라에서 패닝되지 않도록 시작 화면 카메라로 즉시 컷합니다.
	// 이후 ShowLogin/ShowMainMenu의 블렌드는 같은 카메라로의 블렌드라 화면 이동이 보이지 않습니다.
	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		if (ACameraActor* StartCamera = bHasSession ? MainMenuCamera : LoginCamera)
		{
			PlayerController->SetViewTarget(StartCamera);
		}
	}

	if (bHasSession)
	{
		ShowMainMenu();
	}
	else
	{
		ShowLogin();
	}
}

void AShowDownHubFlowManager::ShowLogin()
{
	BlendToCamera(LoginCamera);

	if (!LoginWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoginWidgetClass is not set on ShowDownHubFlowManager."));
		return;
	}

	LoginWidget = CreateWidget<UShowDownLoginWidget>(
		GetPrimaryPlayerController(),
		LoginWidgetClass
	);

	if (!LoginWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create LoginWidget."));
		return;
	}

	LoginWidget->SetUseLegacyNavigation(false);
	LoginWidget->OnLoginSucceeded.AddDynamic(this, &AShowDownHubFlowManager::HandleLoginSucceeded);

	SetActiveWidget(LoginWidget);
	SetUiOnlyInput(LoginWidget);
	OnScreenChanged.Broadcast(EShowDownHubFlowScreen::Login);
}

void AShowDownHubFlowManager::ShowMainMenu()
{
	BlendToCamera(MainMenuCamera);

	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MainMenuWidgetClass is not set on ShowDownHubFlowManager."));
		return;
	}

	MainMenuWidget = CreateWidget<UShowDownMainMenuWidget>(
		GetPrimaryPlayerController(),
		MainMenuWidgetClass
	);

	if (!MainMenuWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create MainMenuWidget."));
		return;
	}

	MainMenuWidget->SetUseLegacyNavigation(false);
	MainMenuWidget->OnSinglePlayRequested.AddDynamic(this, &AShowDownHubFlowManager::HandleSinglePlayRequested);
	MainMenuWidget->OnMultiplayerRequested.AddDynamic(this, &AShowDownHubFlowManager::HandleMultiplayerRequested);
	MainMenuWidget->OnShopRequested.AddDynamic(this, &AShowDownHubFlowManager::HandleShopRequested);
	MainMenuWidget->OnQuitRequested.AddDynamic(this, &AShowDownHubFlowManager::HandleQuitRequested);

	SetActiveWidget(MainMenuWidget);
	SetUiOnlyInput(MainMenuWidget);
	OnScreenChanged.Broadcast(EShowDownHubFlowScreen::MainMenu);
}

void AShowDownHubFlowManager::ShowShop()
{
	BlendToCamera(ShopCamera);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USupabaseSubsystem* SupabaseSubsystem = GameInstance->GetSubsystem<USupabaseSubsystem>())
		{
			// Shop data is loaded on demand instead of during login.
			SupabaseSubsystem->LoadCosmeticData();
		}
	}

	TSubclassOf<UShowDownShopWidget> WidgetClass = ShopWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UShowDownShopWidget::StaticClass();
	}

	ShopWidget = CreateWidget<UShowDownShopWidget>(
		GetPrimaryPlayerController(),
		WidgetClass
	);

	if (!ShopWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create ShopWidget."));
		return;
	}

	ShopWidget->SetUseLegacyBackNavigation(false);
	ShopWidget->OnBackRequested.AddDynamic(this, &AShowDownHubFlowManager::HandleShopBackRequested);

	SetActiveWidget(ShopWidget);
	SetUiOnlyInput(ShopWidget);
	ShopWidget->SetKeyboardFocus();
	OnScreenChanged.Broadcast(EShowDownHubFlowScreen::Shop);
}

void AShowDownHubFlowManager::ShowSinglePlayPreview()
{
	// 메뉴 UI를 걷어내 카드 클릭/베팅 입력이 폰으로 가도록 합니다.
	SetActiveWidget(nullptr);

	APlayerController* PlayerController = GetPrimaryPlayerController();

	// GameCamera가 지정돼 있으면 고정 시네마틱 프레이밍으로 블렌드하고,
	// 비어 있으면 플레이어 폰 카메라로 돌아가 폰의 시점 조작(Turn/LookUp)이 살아납니다.
	if (GameCamera)
	{
		BlendToCamera(GameCamera);
	}
	else if (PlayerController && PlayerController->GetPawn())
	{
		PlayerController->SetViewTargetWithBlend(PlayerController->GetPawn(), CameraBlendTime);
	}

	// 카드 커서 트레이스·카메라 조작·베팅 핫키가 모두 폰에 전달되도록 게임 입력 모드로 전환합니다.
	if (PlayerController)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}

	// 실제 게임 한 판을 시작합니다.
	if (UWorld* World = GetWorld())
	{
		if (AShowDownGameModeBase* GameMode = World->GetAuthGameMode<AShowDownGameModeBase>())
		{
			GameMode->StartSinglePlayer();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("L_Hub GameMode is not AShowDownGameModeBase. Single play cannot start."));
		}
	}

	OnScreenChanged.Broadcast(EShowDownHubFlowScreen::SinglePlayPreview);
	UE_LOG(LogTemp, Log, TEXT("Single play started from HubFlowManager."));
}

void AShowDownHubFlowManager::OpenMultiplayerLevel()
{
	if (MultiplayerLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("MultiplayerLevelName is not set."));
		return;
	}

	UGameplayStatics::OpenLevel(this, MultiplayerLevelName);
}

void AShowDownHubFlowManager::QuitGame()
{
	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		UKismetSystemLibrary::QuitGame(
			this,
			PlayerController,
			EQuitPreference::Quit,
			false
		);
	}
}

void AShowDownHubFlowManager::SetActiveWidget(UUserWidget* NextWidget)
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
	}

	ActiveWidget = NextWidget;

	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport();
	}
}

void AShowDownHubFlowManager::SetUiOnlyInput(UUserWidget* FocusWidget)
{
	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		PlayerController->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;

		if (FocusWidget)
		{
			InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
		}

		PlayerController->SetInputMode(InputMode);
	}
}

void AShowDownHubFlowManager::BlendToCamera(ACameraActor* Camera)
{
	if (!Camera)
	{
		return;
	}

	if (APlayerController* PlayerController = GetPrimaryPlayerController())
	{
		PlayerController->SetViewTargetWithBlend(Camera, CameraBlendTime);
	}
}

APlayerController* AShowDownHubFlowManager::GetPrimaryPlayerController() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

void AShowDownHubFlowManager::HandleLoginSucceeded()
{
	ShowMainMenu();
}

void AShowDownHubFlowManager::HandleSinglePlayRequested()
{
	ShowSinglePlayPreview();
}

void AShowDownHubFlowManager::HandleMultiplayerRequested()
{
	OpenMultiplayerLevel();
}

void AShowDownHubFlowManager::HandleShopRequested()
{
	ShowShop();
}

void AShowDownHubFlowManager::HandleQuitRequested()
{
	QuitGame();
}

void AShowDownHubFlowManager::HandleShopBackRequested()
{
	ShowMainMenu();
}
