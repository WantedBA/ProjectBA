// Copyright TeamBA. All Rights Reserved.

#include "World/InvisibleWall.h"
#include "Components/SphereComponent.h"
#include "Player/BAPlayerCharacter.h"
#include "Components/BoxComponent.h"

AInvisibleWall::AInvisibleWall()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false; // 플레이어가 근접할 때만 Tick

    bStartActive = true; // 일반 투명벽은 항상 활성화

    ProximitySphere = CreateDefaultSubobject<USphereComponent>(TEXT("ProximitySphere"));
    ProximitySphere->SetupAttachment(GetRootComponent());
    ProximitySphere->SetSphereRadius(FadeStartDistance);
    ProximitySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProximitySphere->SetCollisionObjectType(ECC_WorldDynamic);
    ProximitySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    ProximitySphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ProximitySphere->SetGenerateOverlapEvents(true);
}

void AInvisibleWall::BeginPlay()
{
    Super::BeginPlay();

    ProximitySphere->SetSphereRadius(FadeStartDistance);
    ProximitySphere->OnComponentBeginOverlap.AddDynamic(this, &AInvisibleWall::HandleProximityBegin);
    ProximitySphere->OnComponentEndOverlap.AddDynamic(this, &AInvisibleWall::HandleProximityEnd);
}

void AInvisibleWall::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!TrackedActor)
    {
        return;
    }

    FVector ClosestPoint;
    float Distance = BlockingBox->GetDistanceToCollision(TrackedActor->GetActorLocation(), ClosestPoint);
    if (Distance < 0.f)
    {
        Distance = 0.f; // 쿼리 실패시 → 완전 표시
    }

    const float Opacity = FMath::GetMappedRangeValueClamped(
        FVector2D(FadeStartDistance, FadeEndDistance),
        FVector2D(0.f, 1.f),
        Distance);
    SetWallOpacity(Opacity);
}

void AInvisibleWall::HandleProximityBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !OtherActor->IsA(ABAPlayerCharacter::StaticClass()))
    {
        return;
    }

    TrackedActor = OtherActor;
    SetActorTickEnabled(true);
}

void AInvisibleWall::HandleProximityEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor != TrackedActor)
    {
        return;
    }

    TrackedActor = nullptr;
    SetActorTickEnabled(false);
	SetWallOpacity(0.f); // 플레이어가 멀어지면 다시 완전히 투명하게 설정
}
