// Copyright TeamBA. All Rights Reserved.

#include "Instance/QuestManageSubsystem.h"
#include "Tables/BATableManager.h"
#include "Tables/QuestRows.h"
#include "Tables/RewardRows.h"
#include "Enemy/Monster.h"
#include "Enemy/EnemyBase.h"
#include "Quest/QuestTriggerActor.h"

UQuestManageSubsystem* UQuestManageSubsystem::Get(const UObject* WorldContext)
{
    if (!IsValid(WorldContext)) return nullptr;
    const UWorld* World = WorldContext->GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UQuestManageSubsystem>() : nullptr;
}

void UQuestManageSubsystem::StartQuest(int32 QuestTid, const TArray<FTransform>& SpawnTransforms,
    TSubclassOf<AMonster> MonsterClass)
{
    if (ActiveQuests.Contains(QuestTid)) return;

    UBATableManager* TM = UBATableManager::Get(GetGameInstance());
    if (!TM || !TM->FindQuest(QuestTid))
    {
        UE_LOG(LogTemp, Warning, TEXT("[QuestManageSubsystem] Quest %d not found."), QuestTid);
        return;
    }

    FQuestRuntimeState State;

    if (TArray<TWeakObjectPtr<AActor>>* Pending = PendingMonsters.Find(QuestTid))
    {
        for (const TWeakObjectPtr<AActor>& Weak : *Pending)
        {
            AEnemyBase* Enemy = Cast<AEnemyBase>(Weak.Get());
            if (!Enemy) continue;
            
            State.RemainingMonsters++;
            State.SpawnedMonsters.Add(Enemy);
            
            Enemy->OnDeathEvent.AddWeakLambda(this,
                [this, QuestTid]()
                {
                   NotifyMonsterKilled(QuestTid); 
                });
        }
        PendingMonsters.Remove(QuestTid);
    }

    if (TArray<TWeakObjectPtr<AActor>>* PendingW = PendingWalls.Find(QuestTid))
    {
        State.Walls = MoveTemp(*PendingW);
        PendingWalls.Remove(QuestTid);
    }

    for (const FZoneMonsterRows* Row : TM->GetZoneMonstersByQuest(QuestTid))
    {
        if (Row) State.RemainingMonsters += Row->SpawnCount;
    }

    ActiveQuests.Add(QuestTid, MoveTemp(State));
    SetWallsActive(QuestTid, true);
    SpawnQuestMonsters(QuestTid, SpawnTransforms, MonsterClass);

    if (ActiveQuests.Contains(QuestTid) && ActiveQuests[QuestTid].RemainingMonsters <= 0)
    {
        CompleteQuest(QuestTid);
    }
}

void UQuestManageSubsystem::RegisterPrePlacedMonster(int32 QuestTid, AActor* MonsterActor)
{
    if (!IsValid(MonsterActor)) return;
    PendingMonsters.FindOrAdd(QuestTid).Add(MonsterActor);
}

void UQuestManageSubsystem::RegisterWall(int32 QuestTid, AActor* WallActor)
{
    if (!IsValid(WallActor)) return;
    PendingWalls.FindOrAdd(QuestTid).Add(WallActor);
}

void UQuestManageSubsystem::NotifyMonsterKilled(int32 QuestTid)
{
    FQuestRuntimeState* State = ActiveQuests.Find(QuestTid);
    if (!State) return;

    State->RemainingMonsters = FMath::Max(0, State->RemainingMonsters - 1);
    if (State->RemainingMonsters <= 0)
    {
        CompleteQuest(QuestTid);
    }
}

void UQuestManageSubsystem::RegisterTrigger(int32 QuestTid, AQuestTriggerActor* Trigger)
{
    if (QuestTid <= 0 || !IsValid(Trigger)) return;
    RegisteredTriggers.FindOrAdd(QuestTid).AddUnique(Trigger);
}

void UQuestManageSubsystem::AbortQuest(int32 QuestTid)
{
    FQuestRuntimeState* State = ActiveQuests.Find(QuestTid);
    if (!State) return;
    
    for (const TWeakObjectPtr<AActor>& Weak : State->SpawnedMonsters)
    {
        if (AActor* M = Weak.Get())
        {
            M->Destroy();
        }
    }
    
    SetWallsActive(QuestTid, false);
    ActiveQuests.Remove(QuestTid);
    
    if (TArray<TWeakObjectPtr<AQuestTriggerActor>>* Triggers = RegisteredTriggers.Find(QuestTid))
    {
        for (const TWeakObjectPtr<AQuestTriggerActor>& Weak : *Triggers)
        {
            if (AQuestTriggerActor* T = Weak.Get())
            {
                T->ReArm();
            }
        }
    }
}

void UQuestManageSubsystem::SpawnQuestMonsters(int32 QuestTid, const TArray<FTransform>& SpawnTransforms,
                                               TSubclassOf<AMonster> MonsterClass)
{
    if (SpawnTransforms.IsEmpty()) return;
    if (!MonsterClass) MonsterClass = AMonster::StaticClass();

    UWorld* World = GetGameInstance()->GetWorld();
    UBATableManager* TM = UBATableManager::Get(GetGameInstance());
    if (!World || !TM) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    int32 Idx = 0;
    for (const FZoneMonsterRows* Row : TM->GetZoneMonstersByQuest(QuestTid))
    {
        if (!Row) continue;
        for (int32 i = 0; i < Row->SpawnCount; ++i)
        {
            const FTransform& T = SpawnTransforms[Idx % SpawnTransforms.Num()];
            ++Idx;

            AMonster* Spawned = World->SpawnActor<AMonster>(MonsterClass, T, Params);
            if (!Spawned) continue;
            
            if (FQuestRuntimeState* RunningState = ActiveQuests.Find(QuestTid))
            {
                RunningState->SpawnedMonsters.Add(Spawned);
            }
            
            Spawned->OnDeathEvent.AddWeakLambda(this, [this, QuestTid]()
                {
                    NotifyMonsterKilled(QuestTid);
                });

            Spawned->InitializeFromTable(Row->MonsterTid);
        }
    }
}

void UQuestManageSubsystem::CompleteQuest(int32 QuestTid)
{
    if (!ActiveQuests.Contains(QuestTid)) return;

    SetWallsActive(QuestTid, false);

    UBATableManager* TM = UBATableManager::Get(GetGameInstance());
    if (TM)
    {
        if (const FQuestRows* Row = TM->FindQuest(QuestTid))
        {
            ApplyRewards(Row->RewardTid);
        }
    }

    OnQuestCompleted.Broadcast(QuestTid);
    ActiveQuests.Remove(QuestTid);
}

void UQuestManageSubsystem::ApplyRewards(int32 RewardTid)
{
    UBATableManager* TM = UBATableManager::Get(GetGameInstance());
    if (!TM) return;

    for (const FRewardRows* Reward : TM->GetRewardsByTid(RewardTid))
    {
        if (!Reward) continue;
        OnRewardGranted.Broadcast(Reward->RewardType, Reward->Count);
    }
}

void UQuestManageSubsystem::SetWallsActive(int32 QuestTid, bool bActive)
{
    FQuestRuntimeState* State = ActiveQuests.Find(QuestTid);
    if (!State) return;

    for (const TWeakObjectPtr<AActor>& Weak : State->Walls)
    {
        if (AActor* Wall = Weak.Get())
        {
            Wall->SetActorHiddenInGame(!bActive);
            Wall->SetActorEnableCollision(bActive);
        }
    }
}