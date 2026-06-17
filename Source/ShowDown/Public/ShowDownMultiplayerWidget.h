#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShowDownMultiplayerWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShowDownMultiplayerRequest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShowDownJoinRoomRequest, const FString&, RoomCode);

UCLASS()
class SHOWDOWN_API UShowDownMultiplayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Multiplayer")
	FOnShowDownMultiplayerRequest OnHostRequested;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Multiplayer")
	FOnShowDownJoinRoomRequest OnJoinRequested;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|Multiplayer")
	FOnShowDownMultiplayerRequest OnBackRequested;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Multiplayer")
	void ShowStatusMessage(const FString& Message, const FLinearColor& Color);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	UTextBlock* Text_Status;

	UPROPERTY()
	UButton* Button_Host;

	UPROPERTY()
	UButton* Button_Join;

	UPROPERTY()
	UButton* Button_Back;

	UPROPERTY()
	UEditableTextBox* EditableTextBox_RoomCode;

	void BuildDefaultLayout();
	UButton* CreateMenuButton(const FString& Label);

	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleJoinClicked();

	UFUNCTION()
	void HandleBackClicked();
};
