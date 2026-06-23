#include "Card.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "SDPlayerState.h"
#include "ShowDownPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ACard::ACard()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComp);

	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	CardMesh->SetupAttachment(VisualRoot);
	CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CardMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CardMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	// A native fallback keeps multiplayer cards visible even without a BP_Card
	// override. The cube is flattened into an upright playing-card shape.
	if (CardMeshAsset.Succeeded())
	{
		CardMesh->SetStaticMesh(CardMeshAsset.Object);
		CardMesh->SetRelativeScale3D(FVector(0.03f, 0.45f, 0.65f));
	}


	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(RootComp);
	InteractionBounds->SetBoxExtent(InteractionBoundsExtent);
	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	CardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CardText"));
	CardText->SetupAttachment(VisualRoot);
	CardText->SetHorizontalAlignment(EHTA_Center);
	CardText->SetVerticalAlignment(EVRTA_TextCenter);
	CardText->SetTextRenderColor(FColor::Black);
	CardText->SetWorldSize(30.0f);
	CardText->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
}

void ACard::ConfigureInteractionComponents()
{
	if (CardMesh)
	{
		CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CardMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		if (VisualRoot && CardMesh->GetAttachParent() != VisualRoot)
		{
			CardMesh->AttachToComponent(VisualRoot, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	if (CardText && VisualRoot && CardText->GetAttachParent() != VisualRoot)
	{
		CardText->AttachToComponent(VisualRoot, FAttachmentTransformRules::KeepRelativeTransform);
	}

	if (InteractionBounds)
	{
		InteractionBounds->SetBoxExtent(InteractionBoundsExtent);
		InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
		InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		if (RootComp && InteractionBounds->GetAttachParent() != RootComp)
		{
			InteractionBounds->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
}

void ACard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureInteractionComponents();
}

void ACard::BeginPlay()
{
	Super::BeginPlay();

	ConfigureInteractionComponents();
	DefaultLocation = GetActorLocation();
	TargetLocation = DefaultLocation;
	CurrentVisualWorldOffset = FVector::ZeroVector;
	TargetVisualWorldOffset = FVector::ZeroVector;
	DefaultRotation = GetActorRotation();
	TargetRotation = DefaultRotation;
	RefreshVisual();
	SetActorTickEnabled(false);
}

void ACard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTargetTransform();

	CurrentVisualWorldOffset = FMath::VInterpTo(CurrentVisualWorldOffset, TargetVisualWorldOffset, DeltaTime, MoveSpeed);
	if (VisualRoot)
	{
		const FVector VisualRelativeOffset = GetActorTransform().InverseTransformVectorNoScale(CurrentVisualWorldOffset);
		VisualRoot->SetRelativeLocation(VisualRelativeOffset);
	}

	if (!HasAuthority())
	{
		if (CurrentVisualWorldOffset.Equals(TargetVisualWorldOffset, 0.1f))
		{
			CurrentVisualWorldOffset = TargetVisualWorldOffset;
			if (VisualRoot)
			{
				const FVector VisualRelativeOffset = GetActorTransform().InverseTransformVectorNoScale(CurrentVisualWorldOffset);
				VisualRoot->SetRelativeLocation(VisualRelativeOffset);
			}
			SetActorTickEnabled(false);
		}
		return;
	}

	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, MoveSpeed);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, MoveSpeed);
	SetActorLocationAndRotation(NewLocation, NewRotation);
	ForceNetUpdate();

	const bool bVisualAtTarget = CurrentVisualWorldOffset.Equals(TargetVisualWorldOffset, 0.1f);
	const bool bActorAtTarget = GetActorLocation().Equals(TargetLocation, 0.1f)
		&& GetActorRotation().Equals(TargetRotation, 0.1f);
	if (bVisualAtTarget && bActorAtTarget)
	{
		CurrentVisualWorldOffset = TargetVisualWorldOffset;
		if (VisualRoot)
		{
			const FVector VisualRelativeOffset = GetActorTransform().InverseTransformVectorNoScale(CurrentVisualWorldOffset);
			VisualRoot->SetRelativeLocation(VisualRelativeOffset);
		}
		SetActorLocationAndRotation(TargetLocation, TargetRotation);
		ForceNetUpdate();
		SetActorTickEnabled(false);
	}
}

void ACard::SetCard(int32 NewRank)
{
	Rank = FMath::Clamp(NewRank, 1, 7);
	RefreshVisual();
	ForceNetUpdate();
}

void ACard::SetFaceUp(bool bNewFaceUp)
{
	bFaceUp = bNewFaceUp;
	RefreshVisual();
	ForceNetUpdate();
}

void ACard::SetHiddenFromSlot(EShowDownPlayerSlot NewHiddenFromSlot)
{
	HiddenFromSlot = NewHiddenFromSlot;
	RefreshVisual();
	ForceNetUpdate();
}

void ACard::SetHandOwnerSlot(EShowDownPlayerSlot NewHandOwnerSlot)
{
	HandOwnerSlot = NewHandOwnerSlot;
	RefreshVisual();
	ForceNetUpdate();
}

void ACard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACard, Rank);
	DOREPLIFETIME(ACard, bSelectable);
	DOREPLIFETIME(ACard, bFaceUp);
	DOREPLIFETIME(ACard, HiddenFromSlot);
	DOREPLIFETIME(ACard, HandOwnerSlot);
}

void ACard::OnRep_CardVisual()
{
	RefreshVisual();
}

void ACard::RefreshVisual()
{
	if (!CardText)
	{
		return;
	}

	bool bVisibleToLocalPlayer = true;
	const ASDPlayerState* LocalPlayerState = nullptr;
	if (const APlayerController* LocalPlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		LocalPlayerState = LocalPlayerController->GetPlayerState<ASDPlayerState>();
	}

	if (HiddenFromSlot != EShowDownPlayerSlot::None && LocalPlayerState)
	{
		bVisibleToLocalPlayer = bVisibleToLocalPlayer && LocalPlayerState->ShowDownSlot != HiddenFromSlot;
	}

	const bool bShouldShowText = bFaceUp && bVisibleToLocalPlayer;
	if (!bHasCachedVisual || bCachedVisualVisible != bShouldShowText)
	{
		CardText->SetVisibility(bShouldShowText);
		bCachedVisualVisible = bShouldShowText;
	}

	if (!bHasCachedVisual || CachedVisualRank != Rank)
	{
		CardText->SetText(FText::AsNumber(Rank));
		CachedVisualRank = Rank;
	}

	bHasCachedVisual = true;
}

void ACard::SelectCard(bool bNewSelected)
{
	bSelected = bNewSelected;
	UpdateTargetTransform();
	EnableMotionTick();
}

bool ACard::IsSelected() const
{
	return bSelected;
}

void ACard::SetHovered(bool bNewHovered)
{
	bHovered = bNewHovered && bSelectable;
	UpdateTargetTransform();
	EnableMotionTick();
}

bool ACard::IsHovered() const
{
	return bHovered;
}

void ACard::SetSelectable(bool bNewSelectable)
{
	bSelectable = bNewSelectable;
	if (!bSelectable)
	{
		bSelected = false;
		bHovered = false;
		UpdateTargetTransform();
		EnableMotionTick();
	}
	ForceNetUpdate();
}

bool ACard::IsCardSelectable() const
{
	return bSelectable;
}

void ACard::MoveToSlot(USceneComponent* Slot, bool bNewFaceUp)
{
	if (!Slot)
	{
		return;
	}

	bSelected = false;
	bHovered = false;
	SetSelectable(false);
	DefaultLocation = Slot->GetComponentLocation();
	DefaultRotation = Slot->GetComponentRotation();
	UpdateTargetTransform();
	EnableMotionTick();
	SetFaceUp(bNewFaceUp);
	ForceNetUpdate();
}

void ACard::MoveToHandTransform(const FTransform& NewTransform)
{
	DefaultLocation = NewTransform.GetLocation();
	DefaultRotation = NewTransform.GetRotation().Rotator();
	UpdateTargetTransform();
	EnableMotionTick();
	ForceNetUpdate();
}

void ACard::EnableMotionTick()
{
	SetActorTickEnabled(true);
}

void ACard::UpdateTargetTransform()
{
	TargetRotation = DefaultRotation;
	TargetLocation = DefaultLocation;

	if (bSelected)
	{
		TargetVisualWorldOffset = SelectedOffset;
		return;
	}

	if (bHovered)
	{
		TargetVisualWorldOffset = HoverOffset;
		return;
	}

	TargetVisualWorldOffset = FVector::ZeroVector;
}

bool ACard::CanInteract_Implementation(AActor* Interactor) const
{
	return bSelectable;
}

void ACard::Interact_Implementation(AActor* Interactor)
{
	if (!bSelectable)
	{
		return;
	}

	AShowDownPlayerController* ShowDownController = Cast<AShowDownPlayerController>(Interactor);
	if (!ShowDownController)
	{
		ShowDownController = Cast<AShowDownPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	}

	if (ShowDownController)
	{
		ShowDownController->SubmitSelectedCard(this);
	}
}
