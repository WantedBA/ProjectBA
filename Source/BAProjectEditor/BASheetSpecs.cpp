// Copyright TeamBA. All Rights Reserved.

#include "BASheetSpecs.h"

#include "Tables/BossMonster.h"
#include "Tables/ItemRows.h"
#include "Tables/SkillRows.h"

namespace
{
	const TMap<FString, FBASheetSpec>& GetSpecMap()
	{
		static const TMap<FString, FBASheetSpec> Map = {
			{ TEXT("Consume"), { FConsumeItemRow::StaticStruct(), TEXT("ItemTid") } },
			{ TEXT("Skill"), { FSkillRow::StaticStruct(), TEXT("SkillTid") } },
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
