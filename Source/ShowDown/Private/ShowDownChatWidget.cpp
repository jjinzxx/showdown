#include "ShowDownChatWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "PlayerPawn.h"
#include "ShowDownPlayerController.h"
#include "ShowDownGameStateBase.h"

void UShowDownChatWidget::SetOwningShowDownPawn(APlayerPawn* InOwningPawn)
{
	OwningShowDownPawn = InOwningPawn;
}

void UShowDownChatWidget::SetOwningShowDownController(AShowDownPlayerController* InOwningController)
{
	OwningShowDownController = InOwningController;
}

void UShowDownChatWidget::FocusChatInput()
{
	if (EditableTextBox_ChatInput)
	{
		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			EditableTextBox_ChatInput->SetUserFocus(PlayerController);
		}
		EditableTextBox_ChatInput->SetKeyboardFocus();
	}
}

void UShowDownChatWidget::AppendChatLine(const FString& Speaker, const FString& Message)
{
	const FString TrimmedMessage = Message.TrimStartAndEnd();
	if (TrimmedMessage.IsEmpty())
	{
		return;
	}

	if (!ChatHistory.IsEmpty())
	{
		ChatHistory += TEXT("\n");
	}

	ChatHistory += FString::Printf(TEXT("%s: %s"), *Speaker, *TrimmedMessage);
	if (Text_ChatHistory)
	{
		Text_ChatHistory->SetText(FText::FromString(ChatHistory));
	}
	if (ScrollBox_ChatHistory)
	{
		ScrollBox_ChatHistory->ScrollToEnd();
	}
}

void UShowDownChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Send)
	{
		Button_Send->OnClicked.AddDynamic(this, &UShowDownChatWidget::HandleSendClicked);
	}

	if (EditableTextBox_ChatInput)
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &UShowDownChatWidget::HandleInputCommitted);
	}

	if (AShowDownGameStateBase* ShowDownGameState = GetWorld() ? GetWorld()->GetGameState<AShowDownGameStateBase>() : nullptr)
	{
		ShowDownGameState->OnCollectorLLMDecision.AddDynamic(this, &UShowDownChatWidget::HandleCollectorLLMDecision);
		ShowDownGameState->OnCollectorLLMStatus.AddDynamic(this, &UShowDownChatWidget::HandleCollectorLLMStatus);
		ShowDownGameState->OnChatMessageReceived.AddDynamic(this, &UShowDownChatWidget::HandleChatMessageReceived);
	}

	if (Border_ChatHistoryBackground)
	{
		Border_ChatHistoryBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
	}

	SetStatusMessage(TEXT("Enter: chat"), FLinearColor::White);
}

void UShowDownChatWidget::NativeDestruct()
{
	if (Button_Send)
	{
		Button_Send->OnClicked.RemoveDynamic(this, &UShowDownChatWidget::HandleSendClicked);
	}

	if (EditableTextBox_ChatInput)
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &UShowDownChatWidget::HandleInputCommitted);
	}

	if (AShowDownGameStateBase* ShowDownGameState = GetWorld() ? GetWorld()->GetGameState<AShowDownGameStateBase>() : nullptr)
	{
		ShowDownGameState->OnCollectorLLMDecision.RemoveDynamic(this, &UShowDownChatWidget::HandleCollectorLLMDecision);
		ShowDownGameState->OnCollectorLLMStatus.RemoveDynamic(this, &UShowDownChatWidget::HandleCollectorLLMStatus);
		ShowDownGameState->OnChatMessageReceived.RemoveDynamic(this, &UShowDownChatWidget::HandleChatMessageReceived);
	}

	Super::NativeDestruct();
}

void UShowDownChatWidget::HandleSendClicked()
{
	SubmitCurrentText();
}

void UShowDownChatWidget::HandleInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		SubmitCurrentText();
	}
}

void UShowDownChatWidget::HandleCollectorLLMDecision(const FString& Dialogue, const FString& Intent, EShowDownBetAction Action, int32 TargetBet)
{
	AppendChatLine(TEXT("Collector"), Dialogue);
}

void UShowDownChatWidget::HandleCollectorLLMStatus(bool bSuccess, const FString& Message)
{
	SetStatusMessage(Message, bSuccess ? FLinearColor::Green : FLinearColor::Red);
}

void UShowDownChatWidget::HandleChatMessageReceived(const FString& SenderName, const FString& Message)
{
	AppendChatLine(SenderName, Message);
}

void UShowDownChatWidget::SetStatusMessage(const FString& Message, const FLinearColor& Color)
{
	if (Text_Status)
	{
		Text_Status->SetText(FText::FromString(Message));
		Text_Status->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UShowDownChatWidget::SubmitCurrentText()
{
	if (!EditableTextBox_ChatInput || (!OwningShowDownController && !OwningShowDownPawn))
	{
		return;
	}

	const FString Message = EditableTextBox_ChatInput->GetText().ToString().TrimStartAndEnd();
	if (Message.IsEmpty())
	{
		FocusChatInput();
		return;
	}

	if (OwningShowDownController)
	{
		OwningShowDownController->SubmitDialogueInput(Message);
	}
	else if (OwningShowDownPawn)
	{
		OwningShowDownPawn->SubmitDialogueInput(Message);
	}
	SetStatusMessage(TEXT("Sent. Check/Raise to make the boss decide."), FLinearColor::Yellow);
	EditableTextBox_ChatInput->SetText(FText::GetEmpty());
	FocusChatInput();
}
