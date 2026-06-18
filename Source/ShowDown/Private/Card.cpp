#include "Card.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "ShowDownPlayerController.h"

ACard::ACard()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	CardMesh->SetupAttachment(RootComp);
	CardMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CardMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CardMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	CardMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(RootComp);
	InteractionBounds->SetBoxExtent(InteractionBoundsExtent);
	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	CardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CardText"));
	CardText->SetupAttachment(RootComp);
	CardText->SetHorizontalAlignment(EHTA_Center);
	CardText->SetVerticalAlignment(EVRTA_TextCenter);
	CardText->SetTextRenderColor(FColor::Black);
	CardText->SetWorldSize(30.0f);
	CardText->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
}

void ACard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (InteractionBounds)
	{
		InteractionBounds->SetBoxExtent(InteractionBoundsExtent);
	}
}

void ACard::BeginPlay()
{
	Super::BeginPlay();

	DefaultLocation = GetActorLocation();
	TargetLocation = DefaultLocation;
	DefaultRotation = GetActorRotation();
	TargetRotation = DefaultRotation;
	RefreshVisual();
}

void ACard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	UpdateTargetTransform();

	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, MoveSpeed);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, MoveSpeed);
	SetActorLocationAndRotation(NewLocation, NewRotation);
}

void ACard::SetCard(int32 NewRank)
{
	Rank = FMath::Clamp(NewRank, 1, 7);
	RefreshVisual();
}

void ACard::SetFaceUp(bool bNewFaceUp)
{
	bFaceUp = bNewFaceUp;
	RefreshVisual();
}

void ACard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACard, Rank);
	DOREPLIFETIME(ACard, bSelectable);
	DOREPLIFETIME(ACard, bFaceUp);
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

	CardText->SetVisibility(bFaceUp);
	CardText->SetText(FText::AsNumber(Rank));
}

void ACard::SelectCard(bool bNewSelected)
{
	bSelected = bNewSelected;
	UpdateTargetTransform();
}

bool ACard::IsSelected() const
{
	return bSelected;
}

void ACard::SetHovered(bool bNewHovered)
{
	bHovered = bNewHovered && bSelectable;
	UpdateTargetTransform();
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
	}
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
	bUseHandCameraResponse = false;
	DefaultLocation = Slot->GetComponentLocation();
	DefaultRotation = Slot->GetComponentRotation();
	UpdateTargetTransform();
	SetFaceUp(bNewFaceUp);
}

void ACard::MoveToHandTransform(
	const FTransform& NewTransform,
	float InCameraFacingStrength,
	float InMaxCameraFacingAngle)
{
	DefaultLocation = NewTransform.GetLocation();
	DefaultRotation = NewTransform.GetRotation().Rotator();
	bUseHandCameraResponse = InCameraFacingStrength > 0.0f && InMaxCameraFacingAngle > 0.0f;
	CameraFacingStrength = FMath::Clamp(InCameraFacingStrength, 0.0f, 1.0f);
	MaxCameraFacingAngle = FMath::Max(0.0f, InMaxCameraFacingAngle);
	UpdateTargetTransform();
}

void ACard::UpdateTargetTransform()
{
	TargetRotation = DefaultRotation;
	if (bUseHandCameraResponse)
	{
		if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			const FVector ToCamera = (CameraManager->GetCameraLocation() - DefaultLocation).GetSafeNormal();
			const FVector ViewFacing = -CameraManager->GetCameraRotation().Vector();
			const FVector DesiredNormal = (ToCamera + ViewFacing * 0.5f).GetSafeNormal();
			const FQuat BaseRotation = DefaultRotation.Quaternion();
			const FVector CardNormal = BaseRotation.GetForwardVector();

			if (!DesiredNormal.IsNearlyZero() && !CardNormal.IsNearlyZero())
			{
				FVector Axis = FVector::ZeroVector;
				float AngleRadians = 0.0f;
				FQuat::FindBetweenNormals(CardNormal, DesiredNormal).ToAxisAndAngle(Axis, AngleRadians);

				if (!Axis.IsNearlyZero() && AngleRadians > KINDA_SMALL_NUMBER)
				{
					const float ResponseAngle = FMath::Min(
						FMath::RadiansToDegrees(AngleRadians) * CameraFacingStrength,
						MaxCameraFacingAngle);
					const FQuat ResponseRotation(Axis.GetSafeNormal(), FMath::DegreesToRadians(ResponseAngle));
					TargetRotation = (ResponseRotation * BaseRotation).Rotator();
				}
			}
		}
	}

	if (bSelected)
	{
		TargetLocation = DefaultLocation + SelectedOffset;
		return;
	}

	if (bHovered)
	{
		TargetLocation = DefaultLocation + HoverOffset;
		return;
	}

	TargetLocation = DefaultLocation;
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
