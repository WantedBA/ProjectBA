// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestEnums.generated.h"

UENUM(BlueprintType)
enum class EQuestType : uint8
{
	ZoneKill = 0,
	BossKill = 1,
};

UENUM(BlueprintType)
enum class ERewardType : uint8
{
	SkillCount  = 0,
	PotionCount = 1,
};
