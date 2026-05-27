// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "QuestManageSubsystem.generated.h"

class AMonster;
class AQuestZoneActor;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuestCompleted, int32 /*QuestTid*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuestRewardGranted, int32 /*RewardType*/, int32 /*Count*/);

struct FQuestRuntimeState
{
    int32 RemainingMonsters = 0;
    TArray<TWeakObjectPtr<AActor>> Activatables;
    TArray<TWeakObjectPtr<AActor>> SpawnedMonsters;    // 런타임 소환 → AbortQuest 시 Destroy
    TArray<TWeakObjectPtr<AActor>> PrePlacedMonsters;  // 사전 배치(보스 등) → Destroy 제외
};

UCLASS()
class BAPROJECT_API UQuestManageSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static UQuestManageSubsystem* Get(const UObject* WorldContext);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
    void StartQuest(int32 QuestTid, const TArray<FTransform>& SpawnTransforms, TSubclassOf<AMonster> MonsterClass);
    void RegisterPrePlacedMonster(int32 QuestTid, AActor* MonsterActor);
    void RegisterActivatable(int32 QuestTid, AActor* Activatable);
    void NotifyMonsterKilled(int32 QuestTid);

    bool IsQuestActive(int32 QuestTid) const { return ActiveQuests.Contains(QuestTid); }

    void RegisterTrigger(int32 QuestTid, AQuestZoneActor* Trigger);
    void AbortQuest(int32 QuestTid);
    void ResetActiveQuests();

    void ClearQuestMonsters(int32 QuestTid); // 모든 몬스터 제거
    void RefreshQuestMonsters(int32 QuestTid, const TArray<FTransform>& SpawnTransforms, TSubclassOf<AMonster> MonsterClass); // 몬스터 재 생성
    void RespawnQuestZoneEnemies(); // 퀘스트존 순회하면서 리스폰, 추후 지역Tid 생기면 해당 값에 따라 건너뜀처리 해야함
    bool IsQuestCompleted(int32 QuestTid) const { return CompletedQuests.Contains(QuestTid); }

    TSet<int32> MakeQuestSaveData() const { return CompletedQuests; }
    void ApplyQuestSaveData(const TSet<int32>& QuestSaveDataTSet);

    FOnQuestCompleted OnQuestCompleted;
    FOnQuestRewardGranted OnRewardGranted;

private:
    void SpawnQuestMonsters(int32 QuestTid, const TArray<FTransform>& SpawnTransforms,
        TSubclassOf<AMonster> MonsterClass);
    void CompleteQuest(int32 QuestTid);
    void ApplyRewards(int32 RewardTid);
    void SetActivatablesActive(int32 QuestTid, bool bActive);

    TMap<int32, FQuestRuntimeState> ActiveQuests;
    TSet<int32> CompletedQuests;

    TMap<int32, TArray<TWeakObjectPtr<AActor>>> AllSpawnedMonsters; // 퀘스트별로 소환된 모든 몬스터, 리셋시 청소용

    TMap<int32, TArray<TWeakObjectPtr<AActor>>> PendingMonsters;
    TMap<int32, TArray<TWeakObjectPtr<AActor>>> PendingActivatables;
    
    TMap<int32, TArray<TWeakObjectPtr<AQuestZoneActor>>> RegisteredTriggers;
};
