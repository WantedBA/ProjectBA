// Copyright TeamBA. All Rights Reserved.


#include "World/InvisibleWallBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
AInvisibleWallBase::AInvisibleWallBase()
{
	PrimaryActorTick.bCanEverTick = false;

    BlockingBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingBox"));
    SetRootComponent(BlockingBox);
    BlockingBox->SetBoxExtent(BoxExtent);
    BlockingBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BlockingBox->SetCollisionObjectType(ECC_WorldStatic);
    BlockingBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    BlockingBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // 플레이어 + 적(둘 다 Pawn) 차단

    WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
    WallMesh->SetupAttachment(BlockingBox);
    WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WallMesh->SetCanEverAffectNavigation(false);
}

void AInvisibleWallBase::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (BlockingBox)
    {
        BlockingBox->SetBoxExtent(BoxExtent);
    }
}

void AInvisibleWallBase::BeginPlay()
{
    Super::BeginPlay();

    if (WallMaterial)
    {
        WallMID = UMaterialInstanceDynamic::Create(WallMaterial, this);
        WallMesh->SetMaterial(0, WallMID);
    }

    SetWallActive(bStartActive);
    SetWallOpacity(0.f);
}

void AInvisibleWallBase::SetWallActive(bool isActive)
{
    bIsActive = isActive;
    BlockingBox->SetCollisionEnabled(
        isActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AInvisibleWallBase::SetWallOpacity(float Opacity)
{
    if (WallMID)
    {
        WallMID->SetScalarParameterValue(OpacityParameterName, Opacity);
    }
}

