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

