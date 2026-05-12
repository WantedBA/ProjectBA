// Copyright TeamBA. All Rights Reserved.

#include "Tables/BATableManager.h"

#include "Constants/BAProjectConstant.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UBATableManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FString TablePath = FString(TablePath::LoadTablePath);
	LoadTable(ConsumeTable,  *(TablePath + TEXT("DT_Item_Consume.DT_Item_Consume")));
	LoadTable(SkillTable, *(TablePath + TEXT("DT_SkillTree_Skill.DT_SkillTree_Skill")));

	LoadTable(MonsterTable,  *(TablePath + TEXT("DT_Monster_Monster.DT_Monster_Monster")));

	LoadTable(BossAttackTable1, *(TablePath + TEXT("DT_BossMonster_1StageBossAttack.DT_BossMonster_1StageBossAttack")));
	LoadTable(BossAttackTable2, *(TablePath + TEXT("DT_BossMonster_2StageBossAttack.DT_BossMonster_2StageBossAttack")));
	LoadTable(BossAttackTable3, *(TablePath + TEXT("DT_BossMonster_3StageBossAttack.DT_BossMonster_3StageBossAttack")));

	for (IBAPostRead* Table : PostReadList)
	{
		Table->PostRead();
	}

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
	if (!WorldContext)
	{
		return nullptr;
	}
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext))
	{
		return GameInstance->GetSubsystem<UBATableManager>();
	}
	return nullptr;
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
