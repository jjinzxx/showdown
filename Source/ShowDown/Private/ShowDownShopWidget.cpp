#include "ShowDownShopWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ShowDownMainMenuWidget.h"
#include "SupabaseSubsystem.h"

TSharedRef<SWidget> UShowDownShopWidget::RebuildWidget()
{
	BuildWidgetTreeIfNeeded();
	return Super::RebuildWidget();
}

void UShowDownShopWidget::SetMainMenuWidget(UShowDownMainMenuWidget* InMainMenuWidget)
{
	MainMenuWidget = InMainMenuWidget;
}

void UShowDownShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()->GetSubsystem<USupabaseSubsystem>())
	{
		SupabaseSubsystem->OnCosmeticDataLoaded.AddDynamic(
			this,
			&UShowDownShopWidget::HandleCosmeticDataLoaded
		);

		SupabaseSubsystem->OnSkinEquipped.AddDynamic(
			this,
			&UShowDownShopWidget::HandleSkinEquipped
		);
	}

	if (ComboBox_Skins)
	{
		ComboBox_Skins->OnSelectionChanged.AddDynamic(
			this,
			&UShowDownShopWidget::HandleSkinSelectionChanged
		);
	}

	if (Button_Equip)
	{
		Button_Equip->OnClicked.AddDynamic(this, &UShowDownShopWidget::HandleEquipClicked);
	}

	if (Button_Refresh)
	{
		Button_Refresh->OnClicked.AddDynamic(this, &UShowDownShopWidget::HandleRefreshClicked);
	}

	if (Button_Back)
	{
		Button_Back->OnClicked.AddDynamic(this, &UShowDownShopWidget::HandleBackClicked);
	}

	RefreshSkinOptions();
}

void UShowDownShopWidget::NativeDestruct()
{
	if (USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()->GetSubsystem<USupabaseSubsystem>())
	{
		SupabaseSubsystem->OnCosmeticDataLoaded.RemoveDynamic(
			this,
			&UShowDownShopWidget::HandleCosmeticDataLoaded
		);

		SupabaseSubsystem->OnSkinEquipped.RemoveDynamic(
			this,
			&UShowDownShopWidget::HandleSkinEquipped
		);
	}

	Super::NativeDestruct();
}

void UShowDownShopWidget::BuildWidgetTreeIfNeeded()
{
	if (!WidgetTree || Text_Title)
	{
		return;
	}

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShopRoot"));
	RootBorder->SetPadding(FMargin(32.0f));
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.92f));
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopContent"));
	RootBorder->SetContent(RootBox);

	Text_Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Title"));
	Text_Title->SetText(FText::FromString(TEXT("Shop")));
	Text_Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	RootBox->AddChildToVerticalBox(Text_Title);

	Text_Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Status"));
	Text_Status->SetText(FText::FromString(TEXT("Select an owned skin to equip.")));
	Text_Status->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	RootBox->AddChildToVerticalBox(Text_Status);

	ComboBox_Skins = WidgetTree->ConstructWidget<UComboBoxString>(
		UComboBoxString::StaticClass(),
		TEXT("ComboBox_Skins")
	);
	RootBox->AddChildToVerticalBox(ComboBox_Skins);

	Text_SelectedSkin = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("Text_SelectedSkin")
	);
	Text_SelectedSkin->SetText(FText::FromString(TEXT("No skin selected.")));
	Text_SelectedSkin->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	RootBox->AddChildToVerticalBox(Text_SelectedSkin);

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("ButtonRow")
	);
	RootBox->AddChildToVerticalBox(ButtonRow);

	Button_Equip = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Equip"));
	UTextBlock* Text_Equip = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Equip"));
	Text_Equip->SetText(FText::FromString(TEXT("Equip")));
	Text_Equip->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	Button_Equip->AddChild(Text_Equip);
	ButtonRow->AddChild(Button_Equip);

	Button_Refresh = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Refresh"));
	UTextBlock* Text_Refresh = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Refresh"));
	Text_Refresh->SetText(FText::FromString(TEXT("Refresh")));
	Text_Refresh->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	Button_Refresh->AddChild(Text_Refresh);
	ButtonRow->AddChild(Button_Refresh);

	Button_Back = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Back"));
	UTextBlock* Text_Back = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Back"));
	Text_Back->SetText(FText::FromString(TEXT("Back")));
	Text_Back->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	Button_Back->AddChild(Text_Back);
	ButtonRow->AddChild(Button_Back);
}

void UShowDownShopWidget::RefreshSkinOptions()
{
	if (!ComboBox_Skins)
	{
		return;
	}

	FString PreviousSelectedSkinId;
	if (const FShowDownSkin* PreviousSelectedSkin = SkinsByOption.Find(SelectedOption))
	{
		PreviousSelectedSkinId = PreviousSelectedSkin->Id;
	}

	ComboBox_Skins->ClearOptions();
	SkinsByOption.Empty();
	SelectedOption.Empty();

	USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USupabaseSubsystem>()
		: nullptr;

	if (!SupabaseSubsystem)
	{
		if (Text_Status)
		{
			Text_Status->SetText(FText::FromString(TEXT("Supabase subsystem not found.")));
		}

		RefreshSelectedSkinText();
		return;
	}

	const TArray<FShowDownSkin> ShopSkins = SupabaseSubsystem->GetShopSkins();

	for (const FShowDownSkin& Skin : ShopSkins)
	{
		const FString OptionText = MakeSkinOptionText(Skin);
		ComboBox_Skins->AddOption(OptionText);
		SkinsByOption.Add(OptionText, Skin);

		if (!PreviousSelectedSkinId.IsEmpty() && Skin.Id == PreviousSelectedSkinId)
		{
			SelectedOption = OptionText;
		}
		else if (SelectedOption.IsEmpty() && SupabaseSubsystem->IsSkinOwned(Skin.Id))
		{
			SelectedOption = OptionText;
		}
	}

	if (SelectedOption.IsEmpty() && ShopSkins.Num() > 0)
	{
		SelectedOption = MakeSkinOptionText(ShopSkins[0]);
	}

	if (!SelectedOption.IsEmpty())
	{
		ComboBox_Skins->SetSelectedOption(SelectedOption);
	}

	if (Text_Status)
	{
		Text_Status->SetText(
			FText::FromString(FString::Printf(TEXT("Loaded %d shop skins."), ShopSkins.Num()))
		);
	}

	RefreshSelectedSkinText();
}

void UShowDownShopWidget::RefreshSelectedSkinText()
{
	if (!Text_SelectedSkin)
	{
		return;
	}

	const FShowDownSkin* SelectedSkin = SkinsByOption.Find(SelectedOption);
	USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USupabaseSubsystem>()
		: nullptr;

	if (!SelectedSkin || !SupabaseSubsystem)
	{
		Text_SelectedSkin->SetText(FText::FromString(TEXT("No skin selected.")));
		return;
	}

	const bool bOwned = SupabaseSubsystem->IsSkinOwned(SelectedSkin->Id);
	const bool bEquipped = SupabaseSubsystem->GetEquippedSkinId(SelectedSkin->Type) == SelectedSkin->Id;

	Text_SelectedSkin->SetText(
		FText::FromString(
			FString::Printf(
				TEXT("%s\nType: %s\nRarity: %s\nPrice: %d\nOwned: %s\nEquipped: %s"),
				*SelectedSkin->Name,
				*SelectedSkin->Type,
				*SelectedSkin->Rarity,
				SelectedSkin->Price,
				bOwned ? TEXT("true") : TEXT("false"),
				bEquipped ? TEXT("true") : TEXT("false")
			)
		)
	);
}

FString UShowDownShopWidget::MakeSkinOptionText(const FShowDownSkin& Skin) const
{
	USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USupabaseSubsystem>()
		: nullptr;

	const bool bOwned = SupabaseSubsystem && SupabaseSubsystem->IsSkinOwned(Skin.Id);
	const bool bEquipped = SupabaseSubsystem && SupabaseSubsystem->GetEquippedSkinId(Skin.Type) == Skin.Id;

	return FString::Printf(
		TEXT("%s%s [%s] %s - %d (%s)"),
		bEquipped ? TEXT("* ") : TEXT(""),
		*Skin.Name,
		*Skin.Type,
		bOwned ? TEXT("Owned") : TEXT("Locked"),
		Skin.Price,
		*Skin.Id
	);
}

void UShowDownShopWidget::HandleSkinSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SelectedOption = SelectedItem;
	RefreshSelectedSkinText();
}

void UShowDownShopWidget::HandleEquipClicked()
{
	const FShowDownSkin* SelectedSkin = SkinsByOption.Find(SelectedOption);

	if (!SelectedSkin)
	{
		if (Text_Status)
		{
			Text_Status->SetText(FText::FromString(TEXT("No skin selected.")));
		}

		return;
	}

	if (USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()->GetSubsystem<USupabaseSubsystem>())
	{
		if (!SupabaseSubsystem->IsSkinOwned(SelectedSkin->Id))
		{
			if (Text_Status)
			{
				Text_Status->SetText(FText::FromString(TEXT("Locked skin cannot be equipped.")));
			}

			return;
		}

		SupabaseSubsystem->EquipSkin(SelectedSkin->Id);
	}
}

void UShowDownShopWidget::HandleRefreshClicked()
{
	if (USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()->GetSubsystem<USupabaseSubsystem>())
	{
		SupabaseSubsystem->LoadCosmeticData();
	}
}

void UShowDownShopWidget::HandleBackClicked()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}

	RemoveFromParent();
}

void UShowDownShopWidget::HandleCosmeticDataLoaded(bool bSuccess, const FString& Message)
{
	if (Text_Status)
	{
		Text_Status->SetText(FText::FromString(Message));
	}

	if (bSuccess)
	{
		RefreshSkinOptions();
	}
}

void UShowDownShopWidget::HandleSkinEquipped(bool bSuccess, const FString& Message)
{
	if (Text_Status)
	{
		Text_Status->SetText(FText::FromString(Message));
	}

	if (bSuccess)
	{
		RefreshSkinOptions();
	}
}
