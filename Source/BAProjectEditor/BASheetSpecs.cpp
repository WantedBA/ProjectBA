// Copyright TeamBA. All Rights Reserved.

#include "BASheetSpecs.h"

#include "Tables/BossMonster.h"
#include "Tables/ItemRows.h"
#include "Tables/MonsterRows.h"
#include "Tables/PatrolPathRow.h"
#include "Tables/PlayerRows.h"
#include "Tables/Text.h"

namespace
{
	const TMap<FString, FBASheetSpec>& GetSpecMap()
	{
		static const TMap<FString, FBASheetSpec> Map = {
			{ TEXT("Consume"), { FConsumeItemRow::StaticStruct(), TEXT("ItemTid") } },
			{ TEXT("Monster"), { FMonsterRows::StaticStruct(), TEXT("MonsterTid") } },
			{ TEXT("PatrolPath"), { FPatrolPathRow::StaticStruct(), TEXT("Tid") } },
			{ TEXT("1StageBossAttack"), { F1StageBossAttackRows::StaticStruct(), TEXT("Tid") } },
			{ TEXT("2StageBossAttack"), { F2StageBossAttackRows::StaticStruct(), TEXT("Tid") } },
			{ TEXT("3StageBossAttack"), { F3StageBossAttackRows::StaticStruct(), TEXT("Tid") } },
			{ TEXT("BaseStat"), { FPlayerBaseStatRow::StaticStruct(), TEXT("Tid") } },
			{ TEXT("ActionData"), { FPlayerActionDataRow::StaticStruct(), TEXT("Tid") } },
			{ TEXT("Text"), { FTextRows::StaticStruct(), TEXT("TextTid") } },
		};
		return Map;
	}
}

const FBASheetSpec* FBASheetSpecs::Find(const FString& SheetName)
{
	return GetSpecMap().Find(SheetName);
}

TArray<FString> FBASheetSpecs::GetAllSheetNames()
{
	TArray<FString> Out;
	GetSpecMap().GetKeys(Out);
	return Out;
}
