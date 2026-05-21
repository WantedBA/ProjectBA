// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tables/ActionRows.h"
#include "ActionComponent.generated.h"

class UStatComponent;

// 액션 시작 시도 결과. 실패 원인은 UI/디버그/입력 버퍼 판단에 사용된다.
UENUM(BlueprintType)
enum class EActionStartResult : uint8
{
	Success,
	TableManagerUnavailable,
	MovesetNotFound,
	ActionDataNotFound,
	AlreadyRunning,
	NotEnoughStamina,
	Cooldown,
	Buffered
};

// 액션이 실제로 시작될 때 ActionTid와 ActionType을 알린다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionStarted, int32, ActionTid, EActionType, ActionType);

// 현재 액션이 완료되거나 중단되어 런타임 상태가 정리될 때 알린다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionCompleted, int32, ActionTid, EActionType, ActionType);

/**
 * 공용 Action 실행 컴포넌트.
 *
 * Player 입력, AI 의사결정, 네트워크 명령처럼 서로 다른 진입점은
 * EActionCommand 하나로 정규화한 뒤 이 컴포넌트에 전달한다.
 *
 * 책임:
 *   - Moveset 테이블에서 Command + 현재 전투 문맥에 맞는 ActionTid 를 찾는다.
 *   - ActionData 테이블에서 비용, 쿨다운, 이동 잠금 같은 실행 규칙을 읽는다.
 *   - 현재 액션 상태, 스태미너, 쿨다운을 검사하고 액션 시작/종료 상태를 관리한다.
 *
 * 라이프사이클:
 *   1) TryStartAction(Command)  -> Moveset 규칙 선택.
 *   2) TryStartActionByTid(Tid) -> ActionData 실행 가능 여부 검사.
 *   3) BeginAction             -> 런타임 상태 갱신, 즉시형 비용 소비, 시작 이벤트 발행.
 *   4) CompleteCurrentAction   -> 애니메이션/노티파이/상위 로직에서 액션 종료를 통지.
 *
 * 애니메이션 재생은 ActionAnimationComponent 가 ActionAnimationData 를 읽어 처리한다.
 * 무적 iframe 등 애니메이션 시간축 판정은 ActionAnimationComponent 가 ActionWindowData 를 읽어 처리한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActionComponent();

	// BeginAction 직후 브로드캐스트되는 액션 시작 이벤트.
	UPROPERTY(BlueprintAssignable, Category = "Action|Event")
	FOnActionStarted OnActionStarted;

	// CompleteCurrentAction에서 브로드캐스트되는 액션 종료 이벤트.
	UPROPERTY(BlueprintAssignable, Category = "Action|Event")
	FOnActionCompleted OnActionCompleted;

	// 현재 Moveset 문맥에서 Command와 Direction에 맞는 액션을 찾아 시작한다.
	UFUNCTION(BlueprintCallable, Category = "Action")
	bool TryStartAction(EActionCommand Command, EActionDirection Direction = EActionDirection::Any);

	// Moveset 검색 없이 ActionTid를 직접 지정해 시작을 시도한다.
	UFUNCTION(BlueprintCallable, Category = "Action")
	bool TryStartActionByTid(int32 ActionTid);

	// 현재 액션을 종료하고 버퍼된 액션이 있으면 이어서 실행을 시도한다.
	UFUNCTION(BlueprintCallable, Category = "Action")
	void CompleteCurrentAction();

	// 피격/사망처럼 외부 상태가 현재 액션을 강제로 끊을 때 사용한다.
	UFUNCTION(BlueprintCallable, Category = "Action")
	void CancelCurrentAction();

	// 애니메이션 윈도우 등에서 현재 액션의 인터럽트 가능 여부를 제어한다.
	UFUNCTION(BlueprintCallable, Category = "Action")
	void SetActiveActionInterruptLocked(bool bNewInterruptLocked);

	// 애니메이션 윈도우 등에서 입력 버퍼 수신 가능 여부를 제어한다.
	UFUNCTION(BlueprintCallable, Category = "Action")
	void SetActiveActionInputBufferOpen(bool bNewInputBufferOpen);

	// 이동 입력이 바뀔 때 버퍼된 액션의 실행 방향을 최신 값으로 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "Action")
	void UpdateBufferedActionDirection(EActionDirection Direction);

	// 현재 액션 Tid가 유효한지 확인한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	bool IsActionRunning() const { return ActiveActionTid != 0; }

	// 현재 실행 중인 액션 Tid를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	int32 GetActiveActionTid() const { return ActiveActionTid; }

	// 현재 실행 중인 액션의 분류를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	EActionType GetActiveActionType() const { return ActiveActionType; }

	// 현재 액션이 사용하는 런타임 상태 플래그를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	EActionRuntimeState GetActionRuntimeState() const { return RuntimeState; }

	// 현재 액션 데이터가 일반 이동을 잠그는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	bool IsMovementLockedByAction() const;

	// 현재 액션이 루트 모션 이동을 사용하는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	bool IsActiveActionUsingRootMotion() const;

	// 현재 액션이 선택된 방향을 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	EActionDirection GetActiveActionDirection() const { return ActiveActionDirection; }

	// 가장 최근 액션 시작 시도 결과를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	EActionStartResult GetLastStartResult() const { return LastStartResult; }

// Moveset 키 
	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void AddMovesetKey(FName NewMovesetKey) { MovesetKeys.Add(NewMovesetKey); }

	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void ResetMovesetKeys() { MovesetKeys = { FName(TEXT("Default")) }; }

	UFUNCTION(BlueprintPure, Category = "Action|Context")
	bool HasMovesetKey(FName InMovesetKey) const { return MovesetKeys.Contains(InMovesetKey); }
	
//
	// 액션 선택에 사용할 전투 태세를 설정한다.
	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void SetCombatStance(ECombatStance NewCombatStance);

	// 액션 선택에 사용할 가드 상태를 설정한다.
	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void SetGuardState(EGuardState NewGuardState);

	// 액션 선택에 사용할 무기 타입을 설정한다.
	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void SetWeaponType(EActionWeaponType NewWeaponType);

	// 현재 전투 태세를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action|Context")
	ECombatStance GetCombatStance() const { return CombatStance; }

	// 현재 가드 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action|Context")
	EGuardState GetGuardState() const { return GuardState; }

	// 현재 무기 타입을 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action|Context")
	EActionWeaponType GetWeaponType() const { return WeaponType; }

	// 현재 액션 Tid에 대응하는 ActionData 행을 반환한다.
	const FActionDataRow* GetActiveActionData() const;

	// 현재 문맥과 가장 잘 맞는 Moveset 행을 찾는다.
	const FMovesetRow* FindBestMoveset(EActionCommand Command, EActionDirection Direction) const;

	// 스태미너, 쿨다운, 실행 중 상태를 검사해 액션 시작 가능 여부를 판단한다.
	bool CanStartAction(const FActionDataRow& ActionData, EActionDirection Direction);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool TryStartActionByTid(int32 ActionTid, EActionDirection Direction);
	void BeginAction(const FActionDataRow& ActionData, EActionDirection Direction);
	void StartCooldown(const FActionDataRow& ActionData);
	void ConsumeInstantCost(const FActionDataRow& ActionData);
	void BufferAction(int32 ActionTid, EActionDirection Direction);
	void ClearBufferedAction();
	void TryStartBufferedAction();
	void RefreshTickEnabled();
	bool IsActionOnCooldown(int32 ActionTid) const;
	EActionRuntimeState GetRuntimeStateForAction(const FActionDataRow& ActionData) const;

	UPROPERTY(EditAnywhere, Category = "Action|Context")
	TSet<FName> MovesetKeys = { FName(TEXT("Default")) };

	UPROPERTY(EditAnywhere, Category = "Action|Context")
	ECombatStance CombatStance = ECombatStance::Relaxed;

	UPROPERTY(EditAnywhere, Category = "Action|Context")
	EGuardState GuardState = EGuardState::None;

	UPROPERTY(EditAnywhere, Category = "Action|Context")
	EActionWeaponType WeaponType = EActionWeaponType::Any;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	int32 ActiveActionTid = 0;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	EActionType ActiveActionType = EActionType::None;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	EActionRuntimeState RuntimeState = EActionRuntimeState::None;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	EActionDirection ActiveActionDirection = EActionDirection::Any;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	bool bActiveActionInterruptLocked = false;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	bool bActiveActionInputBufferOpen = false;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	bool bActiveActionPausedStaminaRecovery = false;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	int32 BufferedActionTid = 0;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	EActionDirection BufferedActionDirection = EActionDirection::Any;

	UPROPERTY(VisibleAnywhere, Category = "Action|Runtime")
	EActionStartResult LastStartResult = EActionStartResult::Success;

	UPROPERTY(Transient)
	TObjectPtr<UStatComponent> CachedStatComponent;

	TMap<int32, float> CooldownRemainingByActionTid;
	bool bConsumingBufferedAction = false;
};
