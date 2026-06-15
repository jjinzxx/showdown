#include "ShowDownHubFlowManager.h"

#include "Camera/CameraActor.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
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
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (const USupabaseSubsystem* SupabaseSubsystem = GameInstance->GetSubsystem<USupabaseSubsystem>())
		{
			if (!SupabaseSubsystem->GetAccessToken().IsEmpty())
			{
				ShowMainMenu();
				return;
			}
		}
	}

	ShowLogin();
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
	// For now this only moves to the table camera inside the hub.
	// The real single-play state/HUD can be attached here later.
	BlendToCamera(GameCamera);
	OnScreenChanged.Broadcast(EShowDownHubFlowScreen::SinglePlayPreview);
	UE_LOG(LogTemp, Log, TEXT("Single play requested from HubFlowManager."));
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
