// Copyright TeamBA. All Rights Reserved.


#include "World/InvisibleWallBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
AInvisibleWallBase::AInvisibleWallBase()
{
	PrimaryActorTick.bCanEverTick = false;

    BlockingBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingBox"));
    SetRootComponent(BlockingBox);
    BlockingBox->SetBoxExtent(FVector(100.f, 10.f, 200.f)); // 기본 크기 (이후 디테일/인스턴스에서 직접 편집)
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

    // BlockingBox는 여기서 건드리지 않음(디테일에서 직접 편집). WallMesh만 BlockingBox 크기에 맞춤.
    if (WallMesh && WallMesh->GetStaticMesh())
    {
        const FVector BoxExtent = BlockingBox->GetUnscaledBoxExtent();
        const FVector MeshExtent = WallMesh->GetStaticMesh()->GetBoundingBox().GetExtent();

        FVector NewScale = FVector::OneVector;
        if (!FMath::IsNearlyZero(MeshExtent.X)) { NewScale.X = BoxExtent.X / MeshExtent.X; }
        if (!FMath::IsNearlyZero(MeshExtent.Y)) { NewScale.Y = BoxExtent.Y / MeshExtent.Y; }
        if (!FMath::IsNearlyZero(MeshExtent.Z)) { NewScale.Z = BoxExtent.Z / MeshExtent.Z; }
        WallMesh->SetRelativeScale3D(NewScale);
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

