// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PlayerRows.h"
#include "SkillRows.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tables/BAPropTable.h"
#include "Tables/ItemRows.h"
#include "Tables/MonsterRows.h"
#include "Tables/BossMonster.h"
#include "Tables/PatrolPathRow.h"
#include "Tables/QuestRows.h"
#include "Tables/RewardRows.h"
#include "BATableManager.generated.h"

/**
 * Initialize() 시점에 /Game/Table 아래 등록된 모든 DataTable 을 로드한 뒤,
 * 등록 순서대로 PostRead 단계를 일괄 실행한다.
 */
UCLASS()
class BAPROJECT_API UBATableManager : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UBATableManager* Get(const UObject* WorldContext);
	
#if WITH_EDITOR
	void ReloadAllTables();
#endif
	
private:
	void LoadAllTables();

public:
	// Player
	FORCEINLINE const FPlayerBaseStatRow* FindPlayerBaseStat() const { return PlayerBaseStatTable.Find(1); } // 플레이어는 1명이므로 매직넘버 고정
	FORCEINLINE const FPlayerActionDataRow* FindPlayerActionData(const int32 InTid) const { return PlayerActionDataTable.Find(InTid); }
	FORCEINLINE const TMap<int32, FPlayerActionDataRow*>& GetPlayerActionDataTable() const { return PlayerActionDataTable.GetMap(); }
	
	// Consume
	UFUNCTION(BlueprintCallable, Category = "BA|Table")
	bool HasConsume(const int32 InTid) const { return ConsumeTable.Has(InTid); }

	const FConsumeItemRow* FindConsume(const int32 InTid) const { return ConsumeTable.Find(InTid); }

	UFUNCTION(BlueprintCallable, Category = "BA|Table", meta = (DisplayName = "Find Consume"))
	bool BP_FindConsume(int32 InTid, FConsumeItemRow& OutRow) const;

	const TMap<int32, FConsumeItemRow*>& GetConsumeMap() const { return ConsumeTable.GetMap(); }
	
	// Skill
	const FSkillRow* FindSkill(const int32 InTid) const { return SkillTable.Find(InTid); }
	
	UFUNCTION(BlueprintCallable, Category = "BA|Table|Skill", meta = (DisplayName = "Find Skill"))
	bool BP_FindSkill(int32 InTid, FSkillRow& OutRow) const;

	const TMap<int32, FSkillRow*>& GetSkillMap() const { return SkillTable.GetMap(); }
	
	// Monster
	const FMonsterRows* FindMonster(const int32 InTid) const { return MonsterTable.Find(InTid); }
	
	const FPatrolPathRow* FindPatrolPath(int32 InPathId) const { return PatrolPathTable.Find(InPathId); }

	const F1StageBossAttackRows* FindBossAttack1(int32 InTid) const { return BossAttackTable1.Find(InTid); }
	const F2StageBossAttackRows* FindBossAttack2(int32 InTid) const { return BossAttackTable2.Find(InTid); }
	const F3StageBossAttackRows* FindBossAttack3(int32 InTid) const { return BossAttackTable3.Find(InTid); }

	const TMap<int32, F1StageBossAttackRows*>& GetBossAttackMap1() const { return BossAttackTable1.GetMap(); }
	const TMap<int32, F2StageBossAttackRows*>& GetBossAttackMap2() const { return BossAttackTable2.GetMap(); }
	const TMap<int32, F3StageBossAttackRows*>& GetBossAttackMap3() const { return BossAttackTable3.GetMap(); }

	// Quest
	const FQuestRows* FindQuest(int32 InTid) const { return QuestTable.Find(InTid); }
	TArray<const FZoneMonsterRows*> GetZoneMonstersByQuest(int32 InQuestTid) const;

	// Reward
	TArray<const FRewardRows*> GetRewardsByTid(int32 InRewardTid) const;

private:
	template<typename RowType, typename KeyType>
	void LoadTable(TBAPropTable<RowType, KeyType>& OutTable, const FString AssetPath);

	TBAPropTable<FPlayerBaseStatRow, int32> PlayerBaseStatTable;
	TBAPropTable<FPlayerActionDataRow, int32> PlayerActionDataTable;
	
	TBAPropTable<FConsumeItemRow, int32> ConsumeTable;
	
	TBAPropTable<FSkillRow, int32> SkillTable;

	TBAPropTable<FMonsterRows, int32> MonsterTable;
	TBAPropTable<FPatrolPathRow, int32> PatrolPathTable;
	
	TBAPropTable<F1StageBossAttackRows, int32> BossAttackTable1;
	TBAPropTable<F2StageBossAttackRows, int32> BossAttackTable2;
	TBAPropTable<F3StageBossAttackRows, int32> BossAttackTable3;

	TBAPropTable<FQuestRows, int32> QuestTable;
	TBAPropTable<FZoneMonsterRows, int32> ZoneMonsterTable;
	TBAPropTable<FRewardRows, int32> RewardTable;

	TArray<IBAPostRead*> PostReadList;

	UPROPERTY()
	TArray<TObjectPtr<UDataTable>> LoadedTables;
	
// 스킬 데이터 정리
	void BuildChildSkillLists();
};
