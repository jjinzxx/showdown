#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShowDownLobbyWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShowDownLobbyRequest);

UCLASS()
class SHOWDOWN_API UShowDownLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Lobby")
	FOnShowDownLobbyRequest OnStartRequested;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Lobby")
	FOnShowDownLobbyRequest OnLeaveRequested;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Lobby")
	void SetLobbyInfo(const FString& RoomCode, bool bIsHost);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Lobby")
	void ShowStatusMessage(const FString& Message, const FLinearColor& Color);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	FString CachedRoomCode;
	bool bCachedIsHost = false;

	UPROPERTY()
	UTextBlock* Text_Title;

	UPROPERTY()
	UTextBlock* Text_Code;

	UPROPERTY()
	UTextBlock* Text_Status;

	UPROPERTY()
	UTextBlock* Text_Players;

	UPROPERTY()
	UButton* Button_Start;

	UPROPERTY()
	UButton* Button_Leave;

	void BuildDefaultLayout();
	UButton* CreateMenuButton(const FString& Label);
	void RefreshLobbyText();
	void RefreshParticipantText();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleLeaveClicked();
};
