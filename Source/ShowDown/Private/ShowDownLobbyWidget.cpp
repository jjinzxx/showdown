#include "ShowDownLobbyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UShowDownLobbyWidget::RebuildWidget()
{
	BuildDefaultLayout();
	return Super::RebuildWidget();
}

void UShowDownLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Start)
	{
		Button_Start->OnClicked.AddDynamic(this, &UShowDownLobbyWidget::HandleStartClicked);
	}

	if (Button_Leave)
	{
		Button_Leave->OnClicked.AddDynamic(this, &UShowDownLobbyWidget::HandleLeaveClicked);
	}

	RefreshLobbyText();
}

void UShowDownLobbyWidget::NativeDestruct()
{
	if (Button_Start)
	{
		Button_Start->OnClicked.RemoveDynamic(this, &UShowDownLobbyWidget::HandleStartClicked);
	}

	if (Button_Leave)
	{
		Button_Leave->OnClicked.RemoveDynamic(this, &UShowDownLobbyWidget::HandleLeaveClicked);
	}

	Super::NativeDestruct();
}

void UShowDownLobbyWidget::SetLobbyInfo(const FString& RoomCode, bool bIsHost)
{
	CachedRoomCode = RoomCode;
	bCachedIsHost = bIsHost;
	RefreshLobbyText();
}

void UShowDownLobbyWidget::ShowStatusMessage(const FString& Message, const FLinearColor& Color)
{
	if (Text_Status)
	{
		Text_Status->SetText(FText::FromString(Message));
		Text_Status->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UShowDownLobbyWidget::BuildDefaultLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (WidgetTree->RootWidget && Text_Title && Text_Code && Text_Status && Button_Start && Button_Leave)
	{
		return;
	}

	Text_Title = nullptr;
	Text_Code = nullptr;
	Text_Status = nullptr;
	Button_Start = nullptr;
	Button_Leave = nullptr;

	UCanvasPanel* CanvasRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = CanvasRoot;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.9f));
	PanelBorder->SetPadding(FMargin(28.0f));

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	PanelBorder->SetContent(RootBox);

	if (UCanvasPanelSlot* PanelSlot = CanvasRoot->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetAutoSize(false);
		PanelSlot->SetSize(FVector2D(460.0f, 320.0f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
	}

	Text_Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Title"));
	Text_Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text_Title->SetJustification(ETextJustify::Center);
	Text_Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 28));

	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(Text_Title))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	Text_Code = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Code"));
	Text_Code->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
	Text_Code->SetJustification(ETextJustify::Center);
	Text_Code->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 24));

	if (UVerticalBoxSlot* CodeSlot = RootBox->AddChildToVerticalBox(Text_Code))
	{
		CodeSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	Button_Start = CreateMenuButton(TEXT("Start Game"));
	if (UVerticalBoxSlot* StartSlot = RootBox->AddChildToVerticalBox(Button_Start))
	{
		StartSlot->SetPadding(FMargin(0.0f, 4.0f));
	}

	Button_Leave = CreateMenuButton(TEXT("Leave"));
	if (UVerticalBoxSlot* LeaveSlot = RootBox->AddChildToVerticalBox(Button_Leave))
	{
		LeaveSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 16.0f));
	}

	Text_Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Status"));
	Text_Status->SetJustification(ETextJustify::Center);
	Text_Status->SetAutoWrapText(true);
	Text_Status->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));

	if (UVerticalBoxSlot* StatusSlot = RootBox->AddChildToVerticalBox(Text_Status))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	}
}

UButton* UShowDownLobbyWidget::CreateMenuButton(const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ButtonText->SetText(FText::FromString(Label));
	ButtonText->SetJustification(ETextJustify::Center);
	ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ButtonText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18));
	Button->SetContent(ButtonText);
	return Button;
}

void UShowDownLobbyWidget::RefreshLobbyText()
{
	if (Text_Title)
	{
		Text_Title->SetText(FText::FromString(bCachedIsHost ? TEXT("Room Created") : TEXT("Waiting Room")));
	}

	if (Text_Code)
	{
		Text_Code->SetText(FText::FromString(FString::Printf(TEXT("Code: %s"), *CachedRoomCode)));
	}

	if (Button_Start)
	{
		Button_Start->SetVisibility(bCachedIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	ShowStatusMessage(
		bCachedIsHost ? TEXT("Share the code, then start when everyone is ready.") : TEXT("Waiting for host to start..."),
		FLinearColor::White
	);
}

void UShowDownLobbyWidget::HandleStartClicked()
{
	ShowStatusMessage(TEXT("Starting game..."), FLinearColor::Yellow);
	OnStartRequested.Broadcast();
}

void UShowDownLobbyWidget::HandleLeaveClicked()
{
	OnLeaveRequested.Broadcast();
}
