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
	// Shop 화면에서 Back을 눌렀을 때 다시 보여줄 MainMenu 위젯을 넘겨받습니다.
	void SetMainMenuWidget(UShowDownMainMenuWidget* InMainMenuWidget);

protected:
	// C++ 전용 위젯이라 WBP가 없습니다.
	// NativeConstruct보다 먼저 호출되는 RebuildWidget에서 WidgetTree를 만들어야 화면에 렌더링됩니다.
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Shop이 키보드 포커스를 가진 동안 carousel 단축키를 처리합니다.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

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
	UButton* Button_Buy;

	UPROPERTY()
	UButton* Button_Refresh;

	UPROPERTY()
	UButton* Button_Back;

	// ComboBox에 표시되는 문자열을 key로 해서 실제 상점 상품 정보를 찾습니다.
	TMap<FString, FShowDownSkin> SkinsByOption;

	// 방향키 carousel은 ComboBox 옵션 순서를 그대로 사용합니다.
	TArray<FString> OrderedSkinOptions;
	FString SelectedOption;
	int32 SelectedSkinIndex = INDEX_NONE;

	// WBP 없이 C++에서 데모 UI 트리를 구성합니다.
	void BuildWidgetTreeIfNeeded();

	// SupabaseSubsystem에 캐시된 skin_sets 데이터를 ComboBox와 carousel 배열에 반영합니다.
	void RefreshSkinOptions();

	// 현재 선택된 상품의 보유/장착/가격 정보를 화면에 표시합니다.
	void RefreshSelectedSkinText();

	// ComboBox에 들어갈 한 줄짜리 상품 표시 문자열을 만듭니다.
	FString MakeSkinOptionText(const FShowDownSkin& Skin) const;

	// 나중에 특정 타입을 숨기고 싶을 때 쓰는 필터 hook입니다.
	bool ShouldShowSkinAsShopOption(const FShowDownSkin& Skin) const;

	// Offset이 -1이면 이전 상품, +1이면 다음 상품으로 이동합니다.
	void SelectSkinByOffset(int32 Offset);

	// ComboBox 선택과 내부 SelectedIndex를 같은 값으로 맞춥니다.
	void SelectSkinOption(const FString& OptionText);

	UFUNCTION()
	void HandleSkinSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleEquipClicked();

	UFUNCTION()
	void HandleBuyClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleCosmeticDataLoaded(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleSkinEquipped(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleSkinSetPurchased(bool bSuccess, const FString& Message);
};
