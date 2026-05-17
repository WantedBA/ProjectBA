// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tables/ActionRows.h"
#include "ActionComponent.generated.h"

class UStatComponent;

UENUM(BlueprintType)
enum class EActionStartResult : uint8
{
	Success,
	TableManagerUnavailable,
	MovesetNotFound,
	ActionDataNotFound,
	AlreadyRunning,
	NotEnoughStamina,
	Cooldown
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionStarted, int32, ActionTid, EActionType, ActionType);
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
 * 공격 판정, 무적 iframe 적용은 ActionWindowData 를 읽는 후속 단계에서 붙인다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActionComponent();

	UPROPERTY(BlueprintAssignable, Category = "Action|Event")
	FOnActionStarted OnActionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Action|Event")
	FOnActionCompleted OnActionCompleted;

	UFUNCTION(BlueprintCallable, Category = "Action")
	bool TryStartAction(EActionCommand Command, EActionDirection Direction = EActionDirection::Any);

	UFUNCTION(BlueprintCallable, Category = "Action")
	bool TryStartActionByTid(int32 ActionTid);

	UFUNCTION(BlueprintCallable, Category = "Action")
	void CompleteCurrentAction();

	UFUNCTION(BlueprintPure, Category = "Action")
	bool IsActionRunning() const { return ActiveActionTid != 0; }

	UFUNCTION(BlueprintPure, Category = "Action")
	int32 GetActiveActionTid() const { return ActiveActionTid; }

	UFUNCTION(BlueprintPure, Category = "Action")
	EActionType GetActiveActionType() const { return ActiveActionType; }

	UFUNCTION(BlueprintPure, Category = "Action")
	EActionRuntimeState GetActionRuntimeState() const { return RuntimeState; }

	UFUNCTION(BlueprintPure, Category = "Action")
	EActionDirection GetActiveActionDirection() const { return ActiveActionDirection; }

	UFUNCTION(BlueprintPure, Category = "Action")
	EActionStartResult GetLastStartResult() const { return LastStartResult; }

	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void SetMovesetKey(FName NewMovesetKey);

	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void SetCombatStance(ECombatStance NewCombatStance);

	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void SetGuardState(EGuardState NewGuardState);

	UFUNCTION(BlueprintCallable, Category = "Action|Context")
	void SetWeaponType(EActionWeaponType NewWeaponType);

	UFUNCTION(BlueprintPure, Category = "Action|Context")
	FName GetMovesetKey() const { return MovesetKey; }

	UFUNCTION(BlueprintPure, Category = "Action|Context")
	ECombatStance GetCombatStance() const { return CombatStance; }

	UFUNCTION(BlueprintPure, Category = "Action|Context")
	EGuardState GetGuardState() const { return GuardState; }

	UFUNCTION(BlueprintPure, Category = "Action|Context")
	EActionWeaponType GetWeaponType() const { return WeaponType; }

	const FActionDataRow* GetActiveActionData() const;
	const FMovesetRow* FindBestMoveset(EActionCommand Command, EActionDirection Direction) const;
	bool CanStartAction(const FActionDataRow& ActionData);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool TryStartActionByTid(int32 ActionTid, EActionDirection Direction);
	void BeginAction(const FActionDataRow& ActionData, EActionDirection Direction);
	void StartCooldown(const FActionDataRow& ActionData);
	void ConsumeInstantCost(const FActionDataRow& ActionData) const;
	void RefreshTickEnabled();
	bool IsActionOnCooldown(int32 ActionTid) const;
	EActionRuntimeState GetRuntimeStateForAction(const FActionDataRow& ActionData) const;

	UPROPERTY(EditAnywhere, Category = "Action|Context")
	FName MovesetKey = FName(TEXT("Default"));

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
	EActionStartResult LastStartResult = EActionStartResult::Success;

	UPROPERTY(Transient)
	TObjectPtr<UStatComponent> CachedStatComponent;

	TMap<int32, float> CooldownRemainingByActionTid;
};
