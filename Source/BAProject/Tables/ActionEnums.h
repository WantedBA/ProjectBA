// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActionEnums.generated.h"

UENUM(BlueprintType)
enum class EActionCategory : uint8
{
	Attack,
	Defence,
	Movement,
	UseItem
};

UENUM(BlueprintType)
enum class EActionType : uint8
{
	None,
	LightAttack,
	HeavyAttack,
	Guard,
	Sprint,
	DodgeRoll,
	Backstep,
	UseConsumable
};

UENUM(BlueprintType)
enum class EActionStaminaCostType : uint8
{
	Instant, // 즉시 소비(일회성)
	PerSecond // 1초당 퍼센티지로 소비(지속성)
};

UENUM(BlueprintType)
enum class EActionCommand : uint8
{
	None,
	LightAttack,
	HeavyAttack,
	Guard,
	Dodge,
	UseConsumable
};

UENUM(BlueprintType)
enum class ECombatStance : uint8
{
	Relaxed,
	Combat
};

UENUM(BlueprintType)
enum class EGuardState : uint8
{
	None,
	Guarding,
	Blocking,
	GuardBroken
};

UENUM(BlueprintType)
enum class EActionRuntimeState : uint8
{
	None,
	Attacking,
	Dodging,
	Guarding,
	UsingItem
};

UENUM(BlueprintType)
enum class EActionWeaponType : uint8
{
	Any,
	Unarmed,
	Sword,
	GreatSword,
	SwordAndShield
};

UENUM(BlueprintType)
enum class EActionDirection : uint8
{
	Any,
	Forward,
	Backward,
	Left,
	Right,
	ForwardLeft,
	ForwardRight,
	BackwardLeft,
	BackwardRight
};

UENUM(BlueprintType)
enum class EActionWindowType : uint8
{
	Invincible,
	Hit,
	Guard,
	Cancel,
	InterruptLock,
	MovementLock,
	InputBuffer
};
