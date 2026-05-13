// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SkillRows.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tables/BAPropTable.h"
#include "Tables/ItemRows.h"
#include "Tables/MonsterRows.h"
#include "Tables/BossMonster.h"
#include "BATableManager.generated.h"

/**
 * Initialize() 시점에 /Game/Table 아래 등록된 모든 DataTable 을 로드한 뒤,
 * 등록 순서대로 PostRead 단계를 일괄 실행한다.
 */
UCLASS()
class BAPROJECT_API UBATableManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UBATableManager* Get(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "BA|Table")
	bool HasConsume(int32 InTid) const { return ConsumeTable.Has(InTid); }

	const FConsumeItemRow* FindConsume(int32 InTid) const { return ConsumeTable.Find(InTid); }

	UFUNCTION(BlueprintCallable, Category = "BA|Table", meta = (DisplayName = "Find Consume"))
	bool BP_FindConsume(int32 InTid, FConsumeItemRow& OutRow) const;

	const TMap<int32, FConsumeItemRow*>& GetConsumeMap() const { return ConsumeTable.GetMap(); }
	
	// 스킬 테이블
	const FSkillRow* FindSkill(int32 InTid) const { return SkillTable.Find(InTid); }
	
	UFUNCTION(BlueprintCallable, Category = "BA|Table|Skill", meta = (DisplayName = "Find Skill"))
	bool BP_FindSkill(int32 InTid, FSkillRow& OutRow) const;

	const TMap<int32, FSkillRow*>& GetSkillMap() const { return SkillTable.GetMap(); }
	
	// 몬스터 테이블
	const FMonsterRows* FindMonster(int32 InTid) const { return MonsterTable.Find(InTid); }
	
	const F1StageBossAttackRows* FindBossAttack1(int32 InTid) const { return BossAttackTable1.Find(InTid); }
	const F2StageBossAttackRows* FindBossAttack2(int32 InTid) const { return BossAttackTable2.Find(InTid); }
	const F3StageBossAttackRows* FindBossAttack3(int32 InTid) const { return BossAttackTable3.Find(InTid); }

	const TMap<int32, F1StageBossAttackRows*>& GetBossAttackMap1() const { return BossAttackTable1.GetMap(); }
	const TMap<int32, F2StageBossAttackRows*>& GetBossAttackMap2() const { return BossAttackTable2.GetMap(); }
	const TMap<int32, F3StageBossAttackRows*>& GetBossAttackMap3() const { return BossAttackTable3.GetMap(); }

private:
	template<typename RowType, typename KeyType>
	void LoadTable(TBAPropTable<RowType, KeyType>& OutTable, const FString AssetPath);

	TBAPropTable<FConsumeItemRow, int32> ConsumeTable;
	
	TBAPropTable<FSkillRow, int32> SkillTable;

	TBAPropTable<FMonsterRows, int32> MonsterTable;
	
	TBAPropTable<F1StageBossAttackRows, int32> BossAttackTable1;
	TBAPropTable<F2StageBossAttackRows, int32> BossAttackTable2;
	TBAPropTable<F3StageBossAttackRows, int32> BossAttackTable3;

	TArray<IBAPostRead*> PostReadList;

	UPROPERTY()
	TArray<TObjectPtr<UDataTable>> LoadedTables;
};
