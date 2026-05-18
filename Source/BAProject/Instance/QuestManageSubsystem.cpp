// Copyright TeamBA. All Rights Reserved.

#include "Instance/QuestManageSubsystem.h"
#include "Tables/BATableManager.h"
#include "Tables/QuestRows.h"
#include "Tables/RewardRows.h"
#include "Enemy/Monster.h"
#include "Enemy/EnemyBase.h"
#include "Quest/QuestActivatable.h"
#include "Quest/QuestTriggerActor.h"
#include "Tables/QuestEnums.h"

UQuestManageSubsystem* UQuestManageSubsystem::Get(const UObject* WorldContext)
{
    if (!IsValid(WorldContext)) return nullptr;
    const UWorld* World = WorldContext->GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UQuestManageSubsystem>() : nullptr;
}

void UQuestManageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // 테스트 로그 코드
#if !UE_BUILD_SHIPPING
    OnQuestCompleted.AddWeakLambda(this, [](int32 Tid)
    {
        UE_LOG(LogTemp, Display, TEXT("[Quest] Quest %d Cleared!"), Tid);
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
                FString::Printf(TEXT("Quest %d Cleared!"), Tid));
        }
    });
    
    OnRewardGranted.AddWeakLambda(this, [](int32 RewardType, int32 Count)
    {
        UE_LOG(LogTemp, Display, TEXT("[Quest] Reward type=%d count=%d"),
            RewardType, Count);
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                FString::Printf(TEXT("REward Type=%d Count=%d!"), RewardType, Count));
        }
    });
#endif
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

    if (TArray<TWeakObjectPtr<AActor>>* PendingA = PendingActivatables.Find(QuestTid))
    {
        State.Activatables = MoveTemp(*PendingA);
        PendingActivatables.Remove(QuestTid);
    }

    for (const FZoneMonsterRows* Row : TM->GetZoneMonstersByQuest(QuestTid))
    {
        if (Row) State.RemainingMonsters += Row->SpawnCount;
    }

    ActiveQuests.Add(QuestTid, MoveTemp(State));
    SetActivatablesActive(QuestTid, true);
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

void UQuestManageSubsystem::RegisterActivatable(int32 QuestTid, AActor* Activatable)
{
    if (!IsValid(Activatable)) return;
    
    if (!Activatable->Implements<<UQuestActivatable>())
    {
        UE_LOG(LogTemp, Warning,
             TEXT("[QuestManageSubsystem] Actor %s does not implement IQuestActivatable, ignored."),
             *Activatable->GetName());
        return;
    }
    
    PendingActivatables.FindOrAdd(QuestTid).Add(Activatable);
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
    
    SetActivatablesActive(QuestTid, false);
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

    SetActivatablesActive(QuestTid, false);

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

void UQuestManageSubsystem::SetActivatablesActive(int32 QuestTid, bool bActive)
{
    FQuestRuntimeState* State = ActiveQuests.Find(QuestTid);
    if (!State) return;

    for (const TWeakObjectPtr<AActor>& Weak : State->Activatables)
    {
        AActor* Actor = Weak.Get();
        
        if (!Actor || !Actor->Implements<UQuestActivatable>())
            continue;
        
        if (bActive)
        {
            IQuestActivatable::Execute_OnQuestActivated(Actor, QuestTid);
        }
        else
        {
            IQuestActivatable::Execute_OnQuestDeactivated(Actor, QuestTid);
        }
    }
}