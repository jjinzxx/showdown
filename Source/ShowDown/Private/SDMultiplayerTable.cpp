#include "SDMultiplayerTable.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ASDMultiplayerTable::ASDMultiplayerTable()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	TableSurface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableSurface"));
	SetRootComponent(TableSurface);
	TableSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableSurface->SetRelativeScale3D(FVector(12.0f, 9.0f, 0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		TableSurface->SetStaticMesh(CubeMesh.Object);
	}
}
