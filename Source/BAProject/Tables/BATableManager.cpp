// Copyright TeamBA. All Rights Reserved.

#include "Tables/BATableManager.h"

#include "Constants/BAProjectConstant.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UBATableManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const FString TablePath = FString(TablePath::LoadTablePath);
	
	// Player
	LoadTable(PlayerBaseStatTable, *(TablePath + TEXT("DT_Player_BaseStat.DT_Player_BaseStat")));
	LoadTable(PlayerActionDataTable, *(TablePath + TEXT("DT_Player_ActionData.DT_Player_ActionData")));
	
	// Skills
	LoadTable(SkillTable, *(TablePath + TEXT("DT_SkillTree_Skill.DT_SkillTree_Skill")));
	
	// Items
	LoadTable(ConsumeTable,  *(TablePath + TEXT("DT_Item_Consume.DT_Item_Consume")));

	// Monster
	LoadTable(MonsterTable,  *(TablePath + TEXT("DT_Monster_Monster.DT_Monster_Monster")));
	LoadTable(PatrolPathTable, *(TablePath + TEXT("DT_PatrolPath_PatrolPath.DT_PatrolPath_PatrolPath")));

	// Boss
	LoadTable(BossAttackTable1, *(TablePath + TEXT("DT_BossMonster_1StageBossAttack.DT_BossMonster_1StageBossAttack")));
	LoadTable(BossAttackTable2, *(TablePath + TEXT("DT_BossMonster_2StageBossAttack.DT_BossMonster_2StageBossAttack")));
	LoadTable(BossAttackTable3, *(TablePath + TEXT("DT_BossMonster_3StageBossAttack.DT_BossMonster_3StageBossAttack")));

	for (IBAPostRead* Table : PostReadList)
	{
		Table->PostRead();
	}
	
	// SkillRow ChildId 생성
	BuildChildSkillLists();

	UE_LOG(LogTemp, Log, TEXT("[BATableManager] %d table(s) loaded."), PostReadList.Num());
}

void UBATableManager::Deinitialize()
{
	PostReadList.Reset();
	LoadedTables.Reset();
	Super::Deinitialize();
}

UBATableManager* UBATableManager::Get(const UObject* WorldContext)
{
		return GEngine ? GEngine->GetEngineSubsystem<UBATableManager>() : nullptr;
}

bool UBATableManager::BP_FindConsume(int32 InTid, FConsumeItemRow& OutRow) const
{
	if (const FConsumeItemRow* Row = ConsumeTable.Find(InTid))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}

bool UBATableManager::BP_FindSkill(int32 InTid, FSkillRow& OutRow) const
{
	if (const FSkillRow* Row = SkillTable.Find(InTid))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}

template<typename RowType, typename KeyType>
void UBATableManager::LoadTable(TBAPropTable<RowType, KeyType>& OutTable, const FString AssetPath)
{
	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *AssetPath);
	if (!DataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BATableManager] DataTable not found: %s"), *AssetPath);
		return;
	}

	LoadedTables.Add(DataTable);
	OutTable.Build(DataTable);
	PostReadList.Add(&OutTable);
}

void UBATableManager::BuildChildSkillLists()
{
	const TMap<int32, FSkillRow*>& SkillMap = SkillTable.GetMap();
	
	for (const TPair<int32, FSkillRow*>& Pair : SkillMap)
	{
		if (Pair.Value)
		{
			Pair.Value->ChildIds.Reset();
		}
	}

	for (const TPair<int32, FSkillRow*>& Pair : SkillMap)
	{
		const int32 SkillTid = Pair.Key;
		const FSkillRow* SkillRow = Pair.Value;

		if (!SkillRow)
		{
			continue;
		}

		for (const int32 PrerequisiteId : SkillRow->PrerequisiteIds)
		{
			FSkillRow* const* ParentRowPtr = SkillMap.Find(PrerequisiteId);
			if (!ParentRowPtr || !(*ParentRowPtr))
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[BATableManager] Skill %d has invalid prerequisite id: %d"),
					SkillTid,
					PrerequisiteId
				);
				continue;
			}

			(*ParentRowPtr)->ChildIds.AddUnique(SkillTid);
		}
	}
}
