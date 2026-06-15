#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "ShowDownHubFlowManager.generated.h"

class ACameraActor;
class APlayerController;
class UShowDownLoginWidget;
class UShowDownMainMenuWidget;
class UShowDownShopWidget;
class UUserWidget;

UENUM(BlueprintType)
enum class EShowDownHubFlowScreen : uint8
{
	Login,
	MainMenu,
	Shop,
	SinglePlayPreview
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShowDownHubFlowScreenChanged,
	EShowDownHubFlowScreen,
	Screen
);

UCLASS()
class SHOWDOWN_API AShowDownHubFlowManager : public AActor
{
	GENERATED_BODY()

public:
	AShowDownHubFlowManager();

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Flow")
	FOnShowDownHubFlowScreenChanged OnScreenChanged;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Flow")
	void ShowLogin();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Flow")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Flow")
	void ShowShop();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Flow")
	void ShowSinglePlayPreview();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Flow")
	void OpenMultiplayerLevel();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Flow")
	void QuitGame();

protected:
	virtual void BeginPlay() override;

private:
	// Login WBP shown when the player has no active session.
	UPROPERTY(EditAnywhere, Category = "ShowDown|UI")
	TSubclassOf<UShowDownLoginWidget> LoginWidgetClass;

	// Main menu WBP shown after login or when returning to the hub.
	UPROPERTY(EditAnywhere, Category = "ShowDown|UI")
	TSubclassOf<UShowDownMainMenuWidget> MainMenuWidgetClass;

	// Defaults to the C++ demo shop and can be replaced with WBP_Shop later.
	UPROPERTY(EditAnywhere, Category = "ShowDown|UI")
	TSubclassOf<UShowDownShopWidget> ShopWidgetClass;

	// Optional cameras for each hub state. Empty values keep the current view.
	UPROPERTY(EditAnywhere, Category = "ShowDown|Camera")
	ACameraActor* LoginCamera;

	UPROPERTY(EditAnywhere, Category = "ShowDown|Camera")
	ACameraActor* MainMenuCamera;

	UPROPERTY(EditAnywhere, Category = "ShowDown|Camera")
	ACameraActor* ShopCamera;

	UPROPERTY(EditAnywhere, Category = "ShowDown|Camera")
	ACameraActor* GameCamera;

	UPROPERTY(EditAnywhere, Category = "ShowDown|Camera")
	float CameraBlendTime = 0.75f;

	// Multiplayer stays in a separate level while the rest of the hub stays together.
	UPROPERTY(EditAnywhere, Category = "ShowDown|Level")
	FName MultiplayerLevelName = TEXT("ShowDownRoom");

	UPROPERTY()
	UShowDownLoginWidget* LoginWidget;

	UPROPERTY()
	UShowDownMainMenuWidget* MainMenuWidget;

	UPROPERTY()
	UShowDownShopWidget* ShopWidget;

	UPROPERTY()
	UUserWidget* ActiveWidget;

	void SetActiveWidget(UUserWidget* NextWidget);
	void SetUiOnlyInput(UUserWidget* FocusWidget);
	void BlendToCamera(ACameraActor* Camera);
	APlayerController* GetPrimaryPlayerController() const;

	UFUNCTION()
	void HandleLoginSucceeded();

	UFUNCTION()
	void HandleSinglePlayRequested();

	UFUNCTION()
	void HandleMultiplayerRequested();

	UFUNCTION()
	void HandleShopRequested();

	UFUNCTION()
	void HandleQuitRequested();

	UFUNCTION()
	void HandleShopBackRequested();
};
