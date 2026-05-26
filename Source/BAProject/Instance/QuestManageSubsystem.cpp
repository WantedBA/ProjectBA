// Copyright TeamBA. All Rights Reserved.

#include "Instance/QuestManageSubsystem.h"
#include "Tables/BATableManager.h"
#include "Tables/QuestRows.h"
#include "Tables/RewardRows.h"
#include "Enemy/Monster.h"
#include "Enemy/EnemyBase.h"
#include "Quest/QuestActivatable.h"
#include "Quest/QuestZoneActor.h"
#include "Tables/QuestEnums.h"
#include "GameFramework/Actor.h"

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
            if (!Enemy || Enemy->IsDead()) continue;
            
            State.RemainingMonsters++;
            State.PrePlacedMonsters.Add(Enemy);
            
            // 중복 바인딩 방지: QM이 이전에 등록한 모든 델리게이트 제거 후 새로 등록
            Enemy->OnDeathEvent.RemoveAll(this);
            Enemy->OnDeathEvent.AddWeakLambda(this,
                [this, QuestTid]()
                {
                   NotifyMonsterKilled(QuestTid); 
                });
        }
        // StartQuest에서는 삭제하지 않음 (사망 후 재진입 시 다시 읽어야 함)
    }

    if (TArray<TWeakObjectPtr<AActor>>* RegActivatables = PendingActivatables.Find(QuestTid))
    {
        State.Activatables = *RegActivatables;
        // StartQuest에서는 삭제하지 않음
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
    
    if (!Activatable->Implements<UQuestActivatable>())
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

void UQuestManageSubsystem::RegisterTrigger(int32 QuestTid, AQuestZoneActor* Trigger)
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
    
    if (TArray<TWeakObjectPtr<AQuestZoneActor>>* Triggers = RegisteredTriggers.Find(QuestTid))
    {
        for (const TWeakObjectPtr<AQuestZoneActor>& Weak : *Triggers)
        {
            if (AQuestZoneActor* T = Weak.Get())
            {
                T->ReArm();
            }
        }
    }
}

void UQuestManageSubsystem::ResetActiveQuests()
{
    TArray<int32> ActiveTids;
    ActiveQuests.GetKeys(ActiveTids);

    for (int32 Tid : ActiveTids)
    {
        AbortQuest(Tid);
    }
}

void UQuestManageSubsystem::ClearQuestMonsters(int32 QuestTid)
{
    TArray<TWeakObjectPtr<AActor>>* Monsters = AllSpawnedMonsters.Find(QuestTid);
    if (Monsters == nullptr)
    {
        return;
    }

    for (auto& WeakM : *Monsters)
    {
        AActor* Monster = WeakM.Get();
        if (Monster == nullptr)
        {
            continue;
        }

        Monster->Destroy();
    }

    Monsters->Empty();
}

void UQuestManageSubsystem::RefreshQuestMonsters(int32 QuestTid, const TArray<FTransform>& SpawnTransforms, TSubclassOf<AMonster> MonsterClass)
{
    FQuestRuntimeState* State = ActiveQuests.Find(QuestTid);
    if (State == nullptr)
    {
        return;
    }

    
    for (auto& WeakM : State->SpawnedMonsters) // 현재 월드에 소환되어 있는 물리 몬스터들만 파괴
    {
        AActor* Monster = WeakM.Get();
        if (Monster == nullptr)
        {
            continue;
        }

        Monster->Destroy();
    }
    State->SpawnedMonsters.Empty();

    SpawnQuestMonsters(QuestTid, SpawnTransforms, MonsterClass); // 새로운 몬스터 인스턴스들을 스폰
}

void UQuestManageSubsystem::RespawnQuestZoneEnemies()
{
    for (auto& Pair : RegisteredTriggers) // 등록된 모든 트리거(존)를 순회하며 리스폰 처리
    {
        for (auto& WeakTrigger : Pair.Value)
        {
            AQuestZoneActor* Trigger = WeakTrigger.Get();
            if (Trigger == nullptr)
            {
                continue;
            }
                
            Trigger->RespawnEnemiesInZone();// 개별 존의 리스폰 로직 호출
        }
    }
}

void UQuestManageSubsystem::ApplyQuestSaveData(const TSet<int32>& QuestSaveDataTSet)
{
    if (QuestSaveDataTSet.IsEmpty() == true)
    {
        return;
    }

    CompletedQuests = QuestSaveDataTSet;
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
            
            AllSpawnedMonsters.FindOrAdd(QuestTid).Add(Spawned); // 전역 추적 목록에 추가 (리셋 시 청소용)

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

    // 처음 클리어할 때만 보상 및 이벤트 처리
    if (!CompletedQuests.Contains(QuestTid))
    {
        CompletedQuests.Add(QuestTid);

        UBATableManager* TM = UBATableManager::Get(GetGameInstance());
        if (TM)
        {
            if (const FQuestRows* Row = TM->FindQuest(QuestTid))
            {
                ApplyRewards(Row->RewardTid);
            }
        }

        OnQuestCompleted.Broadcast(QuestTid);
    }

    SetActivatablesActive(QuestTid, false);
    ActiveQuests.Remove(QuestTid);

    // 주의: Pending 목록은 영구 보관하여 리스폰/휴식 후 재진입 시에도 투명벽 등이 정상 작동하도록 함
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
