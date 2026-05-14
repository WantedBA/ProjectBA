// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PlayerEnums.generated.h"

UENUM(BlueprintType)
enum class EPlayerActionCategory : uint8
{
	Attack,
	Defence,
	Movement
};

UENUM(BlueprintType)
enum class EPlayerStaminaCostType : uint8
{
	Instant, // 즉시 소비(일회성)
	PerSecond // 1초당 퍼센티지로 소비(지속성)
};

