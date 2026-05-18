// Copyright TeamBA. All Rights Reserved.


#include "World/InvisibleWallTrigger.h"
#include "World/TriggerEventVolume.h"

AInvisibleWallTrigger::AInvisibleWallTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    bStartActive = false; // 이벤트 벽은 평소 열려 있음
}

void AInvisibleWallTrigger::BeginPlay()
{
    Super::BeginPlay();

    if (LinkedVolume)
    {
        LinkedVolume->OnVolumeBeginOverlap.AddDynamic(this, &AInvisibleWallTrigger::HandleVolumeOverlap);
    }

    SetWallOpacity(0.f); // 평소 열림 → 비주얼 꺼둠
}

void AInvisibleWallTrigger::HandleVolumeOverlap(AActor* OverlappingActor)
{
    if (bConsumed)
    {
        return;
    }

    bConsumed = true;
    SetWallActive(true);
    SetWallOpacity(1.f); // 물결 머티리얼 표시 → 막혔음을 유저에게 인지시킴
    OnWallClosed.Broadcast(this);
}

void AInvisibleWallTrigger::OpenWall()
{
    SetWallActive(false);
    SetWallOpacity(0.f);
}

