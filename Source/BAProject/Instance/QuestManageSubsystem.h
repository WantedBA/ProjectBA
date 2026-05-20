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
    TArray<TWeakObjectPtr<AActor>> SpawnedMonsters;
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
    
    FOnQuestCompleted OnQuestCompleted;
    FOnQuestRewardGranted OnRewardGranted;

private:
    void SpawnQuestMonsters(int32 QuestTid, const TArray<FTransform>& SpawnTransforms,
        TSubclassOf<AMonster> MonsterClass);
    void CompleteQuest(int32 QuestTid);
    void ApplyRewards(int32 RewardTid);
    void SetActivatablesActive(int32 QuestTid, bool bActive);

    TMap<int32, FQuestRuntimeState> ActiveQuests;
    TMap<int32, TArray<TWeakObjectPtr<AActor>>> PendingMonsters;
    TMap<int32, TArray<TWeakObjectPtr<AActor>>> PendingActivatables;
    
    TMap<int32, TArray<TWeakObjectPtr<AQuestZoneActor>>> RegisteredTriggers;
};
