// Copyright TeamBA. All Rights Reserved.

#include "Tables/BATableManager.h"

#include "Constants/BAProjectConstant.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UBATableManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	LoadAllTables();
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

#if WITH_EDITOR
void UBATableManager::ReloadAllTables()
{
	PostReadList.Reset();
	LoadedTables.Reset();
	
	LoadAllTables();
}
#endif

void UBATableManager::LoadAllTables()
{
	const FString TablePath = FString(TablePath::LoadTablePath);
	
	// Action
	LoadTable(ActionDataTable, *(TablePath + TEXT("DT_Action_ActionData.DT_Action_ActionData")));
	LoadTable(MovesetTable, *(TablePath + TEXT("DT_Action_Moveset.DT_Action_Moveset")));
	LoadTable(ActionAnimationDataTable, *(TablePath + TEXT("DT_Action_ActionAnimationData.DT_Action_ActionAnimationData")));
	LoadTable(ActionWindowDataTable, *(TablePath + TEXT("DT_Action_ActionWindowData.DT_Action_ActionWindowData")));
	LoadTable(ComboTransitionTable, *(TablePath + TEXT("DT_Action_ComboTransition.DT_Action_ComboTransition")));

	// Player
	LoadTable(PlayerBaseStatTable, *(TablePath + TEXT("DT_Player_BaseStat.DT_Player_BaseStat")));
	
	// Skills
	LoadTable(SkillTable, *(TablePath + TEXT("DT_SkillTree_Skill.DT_SkillTree_Skill")));
	LoadTable(SkillModifierTable, *(TablePath + TEXT("DT_SkillTree_SkillModifier.DT_SkillTree_SkillModifier")));
	
	// Items
	LoadTable(ConsumeTable,  *(TablePath + TEXT("DT_Item_Consume.DT_Item_Consume")));

	// Monster
	LoadTable(MonsterTable,  *(TablePath + TEXT("DT_Monster_Monster.DT_Monster_Monster")));
	LoadTable(PatrolPathTable, *(TablePath + TEXT("DT_PatrolPath_PatrolPath.DT_PatrolPath_PatrolPath")));

	// Boss
	LoadTable(BossAttackTable1, *(TablePath + TEXT("DT_BossMonster_1StageBossAttack.DT_BossMonster_1StageBossAttack")));
	LoadTable(BossAttackTable2, *(TablePath + TEXT("DT_BossMonster_2StageBossAttack.DT_BossMonster_2StageBossAttack")));
	LoadTable(BossAttackTable3, *(TablePath + TEXT("DT_BossMonster_3StageBossAttack.DT_BossMonster_3StageBossAttack")));

	// Quest
	LoadTable(QuestTable,      *(TablePath + TEXT("DT_Quest_Quest.DT_Quest_Quest")));
	LoadTable(ZoneMonsterTable, *(TablePath + TEXT("DT_Quest_ZoneMonster.DT_Quest_ZoneMonster")));

	// Reward
	LoadTable(RewardTable, *(TablePath + TEXT("DT_Reward_Reward.DT_Reward_Reward")));

	// Text
	LoadTable(TextTable, *(TablePath + TEXT("DT_Text_Text.DT_Text_Text")));

	for (IBAPostRead* Table : PostReadList)
	{
		Table->PostRead();
	}
	
	// SkillRow ChildId 생성
	BuildChildSkillLists();

	UE_LOG(LogTemp, Log, TEXT("[BATableManager] %d table(s) loaded."), PostReadList.Num());
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
bool UBATableManager::LoadTable(TBAPropTable<RowType, KeyType>& OutTable, const FString AssetPath)
{
	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *AssetPath);
	if (!DataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BATableManager] DataTable not found: %s"), *AssetPath);
		return false;
	}
	if (!DataTable->GetRowStruct() || !DataTable->GetRowStruct()->IsChildOf(RowType::StaticStruct()))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[BATableManager] RowStruct mismatch: %s (expected %s, actual %s)"),
			*AssetPath,
			*RowType::StaticStruct()->GetName(),
			DataTable->GetRowStruct() ? *DataTable->GetRowStruct()->GetName() : TEXT("None")
		);
		return false;
	}

	LoadedTables.Add(DataTable);
	OutTable.Build(DataTable);
	PostReadList.Add(&OutTable);
	return true;
}

TArray<const FZoneMonsterRows*> UBATableManager::GetZoneMonstersByQuest(int32 InQuestTid) const
{
	TArray<const FZoneMonsterRows*> Result;
	for (const TPair<int32, FZoneMonsterRows*>& Pair : ZoneMonsterTable.GetMap())
	{
		if (Pair.Value && Pair.Value->QuestTid == InQuestTid)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<const FRewardRows*> UBATableManager::GetRewardsByTid(int32 InRewardTid) const
{
	TArray<const FRewardRows*> Result;
	for (const TPair<int32, FRewardRows*>& Pair : RewardTable.GetMap())
	{
		if (Pair.Value && Pair.Value->RewardTid == InRewardTid)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
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
