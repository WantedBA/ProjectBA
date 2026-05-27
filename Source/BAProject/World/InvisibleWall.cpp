// Copyright TeamBA. All Rights Reserved.

#include "World/InvisibleWall.h"
#include "Components/BoxComponent.h"
#include "Player/BAPlayerCharacter.h"

AInvisibleWall::AInvisibleWall()
{
    PrimaryActorTick.bCanEverTick = false;

    bStartActive = true;

    ProximityBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ProximityBox"));
    ProximityBox->SetupAttachment(GetRootComponent());
    ProximityBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProximityBox->SetCollisionObjectType(ECC_WorldDynamic);
    ProximityBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    ProximityBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ProximityBox->SetGenerateOverlapEvents(true);
}

void AInvisibleWall::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform); // WallMesh 크기 동기화 포함

    if (ProximityBox && BlockingBox)
    {
        // BlockingBox 크기 + ProximityExpansion으로 감지 영역 결정.
        // BlockingBox 크기를 바꾸면 ProximityBox도 자동으로 맞춰진다.
        ProximityBox->SetBoxExtent(BlockingBox->GetUnscaledBoxExtent() + ProximityExpansion);
    }
}

void AInvisibleWall::BeginPlay()
{
    Super::BeginPlay();

    ProximityBox->OnComponentBeginOverlap.AddDynamic(this, &AInvisibleWall::HandleProximityBegin);
    ProximityBox->OnComponentEndOverlap.AddDynamic(this, &AInvisibleWall::HandleProximityEnd);
}

void AInvisibleWall::HandleProximityBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !OtherActor->IsA(ABAPlayerCharacter::StaticClass()))
    {
        return;
    }

    SetWallOpacity(1.f);
}

void AInvisibleWall::HandleProximityEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || !OtherActor->IsA(ABAPlayerCharacter::StaticClass()))
    {
        return;
    }

    SetWallOpacity(0.f);
}
