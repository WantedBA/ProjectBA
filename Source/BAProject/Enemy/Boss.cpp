#include "Enemy/Boss.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"

ABoss::ABoss()
{
}

void ABoss::BeginPlay()
{
	Super::BeginPlay();

	if (MonsterTid != 0)
	{
		InitializeFromTable(MonsterTid);
	}
}

void ABoss::InitializeFromTable(int32 InTid)
{
	Super::InitializeFromTable(InTid);

	UBATableManager* TableManager = UBATableManager::Get(this);
	if (TableManager == nullptr)
	{
		return;
	}

	const FMonsterRows* Row = TableManager->FindMonster(InTid);
	if (Row)
	{
		LoadBossPatterns(Row->StageType);
	}
}

void ABoss::LoadBossPatterns(int32 StageType)
{
	UBATableManager* TableManager = UBATableManager::Get(this);
	if (TableManager == nullptr)
	{
		return;
	}

	switch (StageType)
	{
	case 1:
		for (auto& Pair : TableManager->GetBossAttackMap1())
		{
			Pattern1.Add(*Pair.Value);
		}
		break;
	case 2:
		for (auto& Pair : TableManager->GetBossAttackMap2())
		{
			Pattern2.Add(*Pair.Value);
		}
		break;
	case 3:
		for (auto& Pair : TableManager->GetBossAttackMap3())
		{
			Pattern3.Add(*Pair.Value);
		}
		break;
	default:
		break;
	}
}
