// Copyright TeamBA. All Rights Reserved.

#include "Tables/BATableManager.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UBATableManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadTable(ConsumeTable, TEXT("/Game/Table/DT_Item_Consume.DT_Item_Consume"));
	LoadTable(ConsumeTable, TEXT("/Game/Table/DT_SkillTree_Skill.DT_SkillTree_Skill"));

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
void UBATableManager::LoadTable(TBAPropTable<RowType, KeyType>& OutTable, const TCHAR* AssetPath)
{
	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, AssetPath);
	if (!DataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BATableManager] DataTable not found: %s"), AssetPath);
		return;
	}

	LoadedTables.Add(DataTable);
	OutTable.Build(DataTable);
	PostReadList.Add(&OutTable);
}
