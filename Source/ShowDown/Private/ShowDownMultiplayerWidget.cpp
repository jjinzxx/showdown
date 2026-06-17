#include "ShowDownMultiplayerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

TSharedRef<SWidget> UShowDownMultiplayerWidget::RebuildWidget()
{
	BuildDefaultLayout();
	return Super::RebuildWidget();
}

void UShowDownMultiplayerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Host)
	{
		Button_Host->OnClicked.AddDynamic(this, &UShowDownMultiplayerWidget::HandleHostClicked);
	}

	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this, &UShowDownMultiplayerWidget::HandleJoinClicked);
	}

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &UShowDownMultiplayerWidget::HandleBackClicked);
	}

	ShowStatusMessage(TEXT("Choose a multiplayer action."), FLinearColor::White);
}

void UShowDownMultiplayerWidget::NativeDestruct()
{
	if (Button_Host)
	{
		Button_Host->OnClicked.RemoveDynamic(this, &UShowDownMultiplayerWidget::HandleHostClicked);
	}

	if (Button_Join)
	{
		Button_Join->OnClicked.RemoveDynamic(this, &UShowDownMultiplayerWidget::HandleJoinClicked);
	}

	if (Button_Back)
	{
		Button_Back->OnClicked.RemoveDynamic(this, &UShowDownMultiplayerWidget::HandleBackClicked);
	}

	Super::NativeDestruct();
}

void UShowDownMultiplayerWidget::ShowStatusMessage(const FString& Message, const FLinearColor& Color)
{
	if (Text_Status)
	{
		Text_Status->SetText(FText::FromString(Message));
		Text_Status->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UShowDownMultiplayerWidget::BuildDefaultLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (WidgetTree->RootWidget && Text_Status && Button_Host && Button_Join && Button_Back && EditableTextBox_RoomCode)
	{
		return;
	}

	Text_Status = nullptr;
	Button_Host = nullptr;
	Button_Join = nullptr;
	Button_Back = nullptr;
	EditableTextBox_RoomCode = nullptr;

	UCanvasPanel* CanvasRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = CanvasRoot;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.88f));
	PanelBorder->SetPadding(FMargin(28.0f));

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	PanelBorder->SetContent(RootBox);

	if (UCanvasPanelSlot* PanelSlot = CanvasRoot->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetAutoSize(false);
		PanelSlot->SetSize(FVector2D(460.0f, 340.0f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Title"));
	TitleText->SetText(FText::FromString(TEXT("Multiplayer")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 28));

	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	Button_Host = CreateMenuButton(TEXT("Create Room"));
	if (UVerticalBoxSlot* HostSlot = RootBox->AddChildToVerticalBox(Button_Host))
	{
		HostSlot->SetPadding(FMargin(0.0f, 4.0f));
	}

	UTextBlock* CodeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_CodeLabel"));
	CodeLabel->SetText(FText::FromString(TEXT("Room Code")));
	CodeLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CodeLabel->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));

	if (UVerticalBoxSlot* CodeLabelSlot = RootBox->AddChildToVerticalBox(CodeLabel))
	{
		CodeLabelSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 2.0f));
	}

	EditableTextBox_RoomCode = WidgetTree->ConstructWidget<UEditableTextBox>(
		UEditableTextBox::StaticClass(),
		TEXT("EditableTextBox_RoomCode")
	);
	EditableTextBox_RoomCode->SetHintText(FText::FromString(TEXT("Enter room code")));

	if (UVerticalBoxSlot* CodeSlot = RootBox->AddChildToVerticalBox(EditableTextBox_RoomCode))
	{
		CodeSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		CodeSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	Button_Join = CreateMenuButton(TEXT("Join Room"));
	if (UVerticalBoxSlot* JoinSlot = RootBox->AddChildToVerticalBox(Button_Join))
	{
		JoinSlot->SetPadding(FMargin(0.0f, 4.0f));
	}

	Button_Back = CreateMenuButton(TEXT("Back"));
	if (UVerticalBoxSlot* BackSlot = RootBox->AddChildToVerticalBox(Button_Back))
	{
		BackSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 16.0f));
	}

	Text_Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Status"));
	Text_Status->SetJustification(ETextJustify::Center);
	Text_Status->SetAutoWrapText(true);
	Text_Status->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));

	if (UVerticalBoxSlot* StatusSlot = RootBox->AddChildToVerticalBox(Text_Status))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
		StatusSlot->SetHorizontalAlignment(HAlign_Fill);
	}
}

UButton* UShowDownMultiplayerWidget::CreateMenuButton(const FString& Label)
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

void UShowDownMultiplayerWidget::HandleHostClicked()
{
	ShowStatusMessage(TEXT("Creating room..."), FLinearColor::Yellow);
	OnHostRequested.Broadcast();
}

void UShowDownMultiplayerWidget::HandleJoinClicked()
{
	const FString RoomCode = EditableTextBox_RoomCode
		? EditableTextBox_RoomCode->GetText().ToString().TrimStartAndEnd()
		: TEXT("");

	if (RoomCode.IsEmpty())
	{
		ShowStatusMessage(TEXT("Enter a room code."), FLinearColor::Red);
		return;
	}

	ShowStatusMessage(TEXT("Searching room..."), FLinearColor::Yellow);
	OnJoinRequested.Broadcast(RoomCode);
}

void UShowDownMultiplayerWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}
