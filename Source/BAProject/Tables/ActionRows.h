// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "Animation/AnimMontage.h"
#include "CoreMinimal.h"
#include "Tables/ActionEnums.h"
#include "Tables/BAPropTable.h"
#include "ActionRows.generated.h"

/**
 * 액션 실행 규칙의 원본 데이터.
 * UActionComponent 가 시작 가능 여부, 쿨다운, 스태미너, 인터럽트 가능 여부를 읽고,
 * 플레이어 Sprint 초기화도 고정 Sprint ActionTid의 일부 비용 필드를 읽는다.
 */
USTRUCT(BlueprintType, meta = (BASheet = "ActionData"))
struct BAPROJECT_API FActionDataRow : public FBARowBase
{
	GENERATED_BODY()

	// UBATableManager::FindActionData의 키. 액션 시작, 쿨다운, 현재 액션 추적에 그대로 사용된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	int32 Tid = 0;

	// 기획/디버그용 이름. 현재 C++ 런타임 로직에서는 직접 참조하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	FName Name = NAME_None;

	// 액션 분류용 값. 현재 C++ 런타임 로직에서는 직접 참조하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	EActionCategory Category = EActionCategory::Movement;

	// BeginAction에서 현재 액션 타입으로 저장되고, 런타임 상태와 시작/종료 이벤트에 사용된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	EActionType ActionType = EActionType::None;

	// CanStartAction의 필요 스태미너 검사와 액션 비용 소비에 사용된다. Sprint와 OnDemand 액션은 각 전용 시점에서 읽는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float StaminaCost = 0.f;

	// Instant는 시작 시, PerSecond는 지속 중, OnDemand는 피격/방어 판정 같은 명시적 시점에 소비한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	EActionStaminaCostType StaminaCostType = EActionStaminaCostType::Instant;

	// 액션 실행 중 적용할 스태미너 회복 배율. 1이면 기본 회복, 0.5면 절반 속도로 회복한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float StaminaRecoveryRateMultiplier = 1.f;

	// 액션 시작 가능 여부와 Sprint 진입 가능 여부를 검사하는 최소 현재 스태미너.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float MinRequiredStamina = 0.f;

	// Sprint 전용 값. 탈진 후 재진입 잠금을 해제할 최대 스태미너 대비 회복 비율이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float SprintRestartStaminaPercent = 0.f;

	// BeginAction 직후 액션 Tid별 쿨다운으로 등록되고 ActionComponent Tick에서 감소한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float Cooldown = 0.f;

	// 현재 C++에서는 직접 참조하지 않는다. 입력 버퍼 개폐는 ActionWindowData의 InputBuffer 윈도우가 담당한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	float InputBufferTime = 0.f;

	// BAPlayerCharacter::HandleActionStarted에서 IsMovementLockedByAction으로 조회한다. 현재 구현은 InterruptLock 상태도 함께 본다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	bool bLocksMovement = false;

	// IsActiveActionUsingRootMotion의 반환값으로 노출된다. 실제 몽타주 루트 모션 적용은 ActionAnimationData.bUseRootMotion이 담당한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionData")
	bool bUsesRootMotion = false;

	// 실행 중인 액션이 다른 액션으로 교체될 수 있는지 판단한다. InterruptLock 윈도우 중에는 이 값과 무관하게 막힌다.
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

	// Moveset 테이블 행 키. 현재 선택 로직은 전체 테이블을 순회하며 조건에 맞는 행을 고른다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	int32 Tid = 0;

	// ActionComponent의 현재 MovesetKey와 비교한다. None이면 모든 MovesetKey에 대한 공통 후보로 취급된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	FName MovesetKey = NAME_None;

	// TryStartAction으로 들어온 입력/AI 명령과 반드시 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	EActionCommand Command = EActionCommand::None;

	// ActionComponent의 현재 전투 태세와 정확히 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	ECombatStance CombatStance = ECombatStance::Relaxed;

	// ActionComponent의 현재 가드 상태와 정확히 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	EGuardState GuardState = EGuardState::None;

	// Any면 무기 공통 후보, 그 외 값은 ActionComponent의 현재 무기 타입과 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	EActionWeaponType WeaponType = EActionWeaponType::Any;

	// Any면 방향 공통 후보, 그 외 값은 TryStartAction에 전달된 방향과 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	EActionDirection Direction = EActionDirection::Any;

	// 조건 매칭 후 실제로 시작을 시도할 ActionData Tid.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Moveset")
	int32 ActionTid = 0;

	// 후보 점수의 1차 기준. 이후 MovesetKey, WeaponType, Direction의 구체성이 가산점으로 붙는다.
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

	// ActionAnimationData 행 키. 활성 애니메이션 Tid로 저장되고 WindowData 연결에 사용된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	int32 Tid = 0;

	// ActionComponent가 시작한 ActionTid와 일치하는 애니메이션 후보만 검색한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	int32 ActionTid = 0;

	// Any면 방향 공통 후보, 그 외 값은 현재 액션 방향과 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	EActionDirection Direction = EActionDirection::Any;

	// ActionComponent의 현재 전투 태세와 정확히 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	ECombatStance CombatStance = ECombatStance::Relaxed;

	// Any면 무기 공통 후보, 그 외 값은 ActionComponent의 현재 무기 타입과 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	EActionWeaponType WeaponType = EActionWeaponType::Any;

	// PlayActionAnimation에서 동기 로드 후 재생할 몽타주.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	TSoftObjectPtr<UAnimMontage> Montage;

	// 몽타주 재생 성공 후 특정 섹션으로 점프할 때 사용한다. None이면 처음부터 재생한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	FName StartSection = NAME_None;

	// Montage_PlayWithBlendSettings에 전달되는 재생 속도. 0 이하이면 1로 보정된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	float PlayRate = 1.f;

	// 몽타주 시작 블렌드 시간. 음수는 0으로 보정된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	float BlendIn = 0.1f;

	// 액션 중단/정지 시 Montage_Stop에 전달되는 블렌드 아웃 시간. 음수는 0으로 보정된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionAnimationData")
	float BlendOut = 0.1f;

	// true면 재생 중 AnimInstance RootMotionMode를 RootMotionFromMontagesOnly로 임시 전환한다.
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

	// ActionWindowData 행 키. 윈도우 열림/닫힘 이벤트의 ActionWindowTid로 전달된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	int32 Tid = 0;

	// 현재 재생 중인 ActionAnimationData Tid와 일치하는 윈도우만 활성 후보가 된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	int32 ActionAnimationTid = 0;

	// Invincible, InterruptLock, InputBuffer는 즉시 런타임 상태를 바꾸고, 나머지는 이벤트로 외부 시스템에 전달된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	EActionWindowType WindowType = EActionWindowType::Invincible;

	// 몽타주 재생 위치가 이 시간에 도달하면 윈도우를 연다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	float StartTime = 0.f;

	// 몽타주 재생 위치가 이 시간 이상이면 윈도우를 닫는다. StartTime 이하인 행은 무시된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	float EndTime = 0.f;

	// 윈도우 이벤트에 그대로 전달되는 추가 데이터. Hit, Guard, Cancel 등 외부 처리용 타입에서 활용할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ActionWindowData")
	FString Payload;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};

// 공격 몽타주 연결 + 다음 콤보 정보
USTRUCT(BlueprintType, meta = (BASheet = "ComboTransition"))
struct BAPROJECT_API FComboTransitionRow : public FBARowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ComboTransition")
	int32 Tid = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ComboTransition")
	TSoftObjectPtr<UAnimMontage> Montage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ComboTransition")
	float PlayRate = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ComboTransition")
	int32 NextOnL = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|ComboTransition")
	int32 NextOnR = 0;
	
	virtual void PostRead() override
	{
		bTid = Tid;
	}
};
