#include "SDCardPlacementAnchor.h"

#include "Components/SceneComponent.h"
#include "UObject/UnrealType.h"

ASDCardPlacementAnchor::ASDCardPlacementAnchor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CardSlot = CreateDefaultSubobject<USceneComponent>(TEXT("CardSlot"));
	CardSlot->SetupAttachment(SceneRoot);
}

USceneComponent* ASDCardPlacementAnchor::GetSlotComponent() const
{
	return CardSlot ? CardSlot.Get() : GetRootComponent();
}

bool ASDCardPlacementAnchor::IsHandAnchor() const
{
	return PlacementRole == ESDCardPlacementRole::PlayerHand || PlacementRole == ESDCardPlacementRole::OpponentHand;
}

bool ASDCardPlacementAnchor::IsForeheadAnchor() const
{
	return PlacementRole == ESDCardPlacementRole::PlayerForehead || PlacementRole == ESDCardPlacementRole::OpponentForehead;
}

#if WITH_EDITOR
bool ASDCardPlacementAnchor::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	const FName PropertyName = InProperty ? InProperty->GetFName() : NAME_None;
	const bool bHandOnlyProperty =
		PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, HandLayoutStyle)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, FanWidth)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, FanDistance)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, GripToCenterDistance)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, AnglePerGap)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, MaxFanAngle)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, LayerStep)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, FaceTiltAngle)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, EdgeCurlAngle)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, CameraFacingStrength)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, MaxCameraFacingAngle)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, SelectedOffset)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, HoverOffset)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASDCardPlacementAnchor, MoveSpeed);

	return !bHandOnlyProperty || IsHandAnchor();
}
#endif

ASDPlayerHandAnchor::ASDPlayerHandAnchor()
{
	PlacementRole = ESDCardPlacementRole::PlayerHand;
}

ASDPlayerForeheadAnchor::ASDPlayerForeheadAnchor()
{
	PlacementRole = ESDCardPlacementRole::PlayerForehead;
}

ASDOpponentHandAnchor::ASDOpponentHandAnchor()
{
	PlacementRole = ESDCardPlacementRole::OpponentHand;
}

ASDOpponentForeheadAnchor::ASDOpponentForeheadAnchor()
{
	PlacementRole = ESDCardPlacementRole::OpponentForehead;
}
