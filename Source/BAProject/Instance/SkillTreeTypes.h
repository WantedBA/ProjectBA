#pragma once

#include "CoreMinimal.h"
#include "SkillTreeTypes.generated.h"

// 게임 중 스킬 상태
UENUM(BlueprintType)
enum class ESkillNodeState : uint8
{
	Locked		UMETA(DisplayName = "Locked"),
	Available	UMETA(DisplayName = "Available"),
	Activated	UMETA(DisplayName = "Activated"),
};
