#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SupabaseSubsystem.h"
#include "Types/SlateEnums.h"
#include "ShowDownShopWidget.generated.h"

class UButton;
class UComboBoxString;
class UShowDownMainMenuWidget;
class UTextBlock;
class UVerticalBox;

UCLASS()
class SHOWDOWN_API UShowDownShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMainMenuWidget(UShowDownMainMenuWidget* InMainMenuWidget);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	UShowDownMainMenuWidget* MainMenuWidget;

	UPROPERTY()
	UTextBlock* Text_Title;

	UPROPERTY()
	UTextBlock* Text_Status;

	UPROPERTY()
	UTextBlock* Text_SelectedSkin;

	UPROPERTY()
	UComboBoxString* ComboBox_Skins;

	UPROPERTY()
	UButton* Button_Equip;

	UPROPERTY()
	UButton* Button_Refresh;

	UPROPERTY()
	UButton* Button_Back;

	TMap<FString, FShowDownSkin> SkinsByOption;
	FString SelectedOption;

	void BuildWidgetTreeIfNeeded();
	void RefreshSkinOptions();
	void RefreshSelectedSkinText();
	FString MakeSkinOptionText(const FShowDownSkin& Skin) const;

	UFUNCTION()
	void HandleSkinSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleEquipClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleCosmeticDataLoaded(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleSkinEquipped(bool bSuccess, const FString& Message);
};
