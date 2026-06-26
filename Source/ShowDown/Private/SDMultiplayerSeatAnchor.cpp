#include "SDMultiplayerSeatAnchor.h"

#include "Components/SceneComponent.h"

ASDMultiplayerSeatAnchor::ASDMultiplayerSeatAnchor()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	HandSlot = CreateDefaultSubobject<USceneComponent>(TEXT("HandSlot"));
	HandSlot->SetupAttachment(SceneRoot);
	// Cards sit on the near side of the table, not below the camera. The
	// previous -90 Z offset put the hand beneath the tabletop in this map.
	HandSlot->SetRelativeLocation(FVector(250.0f, 0.0f, 25.0f));
	HandSlot->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	ForeheadSlot = CreateDefaultSubobject<USceneComponent>(TEXT("ForeheadSlot"));
	ForeheadSlot->SetupAttachment(SceneRoot);
	ForeheadSlot->SetRelativeLocation(FVector(620.0f, 0.0f, -20.0f));
	ForeheadSlot->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
}

void ASDMultiplayerSeatAnchor::ConfigureFromCameraTransform(const FTransform& CameraTransform)
{
	SetActorTransform(CameraTransform);
}
