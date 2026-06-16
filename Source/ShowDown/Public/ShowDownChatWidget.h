#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShowDownTypes.h"
#include "ShowDownChatWidget.generated.h"

class APlayerPawn;
class UBorder;
class UButton;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;

UCLASS()
class SHOWDOWN_API UShowDownChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOwningShowDownPawn(APlayerPawn* InOwningPawn);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Chat")
	void FocusChatInput();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|Chat")
	void AppendChatLine(const FString& Speaker, const FString& Message);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	APlayerPawn* OwningShowDownPawn = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox* EditableTextBox_ChatInput = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Button_Send = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_ChatHistory = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* ScrollBox_ChatHistory = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_Status = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* Border_ChatHistoryBackground = nullptr;

	UPROPERTY()
	FString ChatHistory;

	UFUNCTION()
	void HandleSendClicked();

	UFUNCTION()
	void HandleInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleCollectorLLMDecision(const FString& Dialogue, const FString& Intent, EShowDownBetAction Action, int32 TargetBet);

	UFUNCTION()
	void HandleCollectorLLMStatus(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleChatMessageReceived(const FString& SenderName, const FString& Message);

	void SetStatusMessage(const FString& Message, const FLinearColor& Color);
	void SubmitCurrentText();
};
