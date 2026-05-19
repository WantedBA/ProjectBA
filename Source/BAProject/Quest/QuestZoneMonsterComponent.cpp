// Copyright TeamBA. All Rights Reserved.

#include "Quest/QuestZoneMonsterComponent.h"
#include "Instance/QuestManageSubsystem.h"

// Sets default values
UQuestZoneMonsterComponent::UQuestZoneMonsterComponent()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void UQuestZoneMonsterComponent::BeginPlay()
{
    Super::BeginPlay();

    if (QuestTid <= 0) return;

    if (UQuestManageSubsystem* QM = UQuestManageSubsystem::Get(this))
    {
        QM->RegisterPrePlacedMonster(QuestTid, GetOwner());
    }
}