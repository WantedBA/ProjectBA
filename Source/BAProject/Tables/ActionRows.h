// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "Animation/AnimMontage.h"
#include "CoreMinimal.h"
#include "Tables/ActionEnums.h"
#include "Tables/BAPropTable.h"
#include "ActionRows.generated.h"

USTRUCT(BlueprintType, meta = (BASheet = "ActionData"))
struct BAPROJECT_API FActionDataRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	EActionCategory Category = EActionCategory::Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	EActionType ActionType = EActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float StaminaCost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	EActionStaminaCostType StaminaCostType = EActionStaminaCostType::Instant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float MinRequiredStamina = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float SprintRestartStaminaPercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float Cooldown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float InputBufferTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	bool bLocksMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	bool bUsesRootMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	bool bCanBeInterrupted = true;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};

/**
 * 입력 또는 AI 의사결정으로 나온 명령을 실제 ActionData 로 해석하는 규칙.
 * 같은 명령이라도 전투 태세, 가드 상태, 무기, 방향에 따라 다른 ActionTid 를 고를 수 있다.
 */
USTRUCT(BlueprintType, meta = (BASheet = "Moveset"))
struct BAPROJECT_API FMovesetRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	FName MovesetKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	EActionCommand Command = EActionCommand::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	ECombatStance CombatStance = ECombatStance::Relaxed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	EGuardState GuardState = EGuardState::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	EActionWeaponType WeaponType = EActionWeaponType::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	EActionDirection Direction = EActionDirection::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	int32 ActionTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	int32 Priority = 0;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};

/**
 * 하나의 ActionData 에 연결되는 애니메이션 변형.
 * 방향/태세/무기에 따라 다른 몽타주 또는 섹션을 선택할 수 있다.
 */
USTRUCT(BlueprintType, meta = (BASheet = "ActionAnimationData"))
struct BAPROJECT_API FActionAnimationDataRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	int32 ActionTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	EActionDirection Direction = EActionDirection::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	ECombatStance CombatStance = ECombatStance::Relaxed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	EActionWeaponType WeaponType = EActionWeaponType::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	FName StartSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	float BlendIn = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	float BlendOut = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	bool bUseRootMotion = false;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};

/**
 * 애니메이션 시간축에 붙는 액션 판정 구간.
 * 구르기 무적 iframe, 공격 히트 프레임, 캔슬 가능 구간처럼 타이밍이 중요한 정보는 여기에 둔다.
 */
USTRUCT(BlueprintType, meta = (BASheet = "ActionWindowData"))
struct BAPROJECT_API FActionWindowDataRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	int32 ActionAnimationTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	EActionWindowType WindowType = EActionWindowType::Invincible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	float StartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	float EndTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	FString Payload;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};
