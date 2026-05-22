// Copyright TeamBA. All Rights Reserved.

#include "Component/ActionComponent.h"

#include "Component/StatComponent.h"
#include "Tables/BATableManager.h"

namespace
{
	constexpr int32 ActionComponentInvalidActionTid = 0;
	const FName ActionStaminaRecoveryPauseSource(TEXT("Action"));
	const FName ActionStaminaRecoveryRateMultiplierSource(TEXT("Action"));

	bool IsGuardInterruptActionType(const EActionType ActionType)
	{
		return ActionType == EActionType::DodgeRoll
			|| ActionType == EActionType::LightAttack
			|| ActionType == EActionType::HeavyAttack
			|| ActionType == EActionType::Backstep
			|| ActionType == EActionType::Sprint
			|| ActionType == EActionType::UseConsumable;
	}
}

UActionComponent::UActionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UActionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		CachedStatComponent = Owner->FindComponentByClass<UStatComponent>();
	}
}

void UActionComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TArray<int32> ExpiredCooldowns;
	for (TPair<int32, float>& Pair : CooldownRemainingByActionTid)
	{
		Pair.Value = FMath::Max(0.f, Pair.Value - DeltaTime);
		if (Pair.Value <= 0.f)
		{
			ExpiredCooldowns.Add(Pair.Key);
		}
	}

	for (const int32 ActionTid : ExpiredCooldowns)
	{
		CooldownRemainingByActionTid.Remove(ActionTid);
	}

	RefreshTickEnabled();
}

bool UActionComponent::TryStartAction(const EActionCommand Command, const EActionDirection Direction)
{
	const FMovesetRow* Moveset = FindBestMoveset(Command, Direction);
	if (!Moveset)
	{
		LastStartResult = EActionStartResult::MovesetNotFound;
		return false;
	}

	return TryStartActionByTid(Moveset->ActionTid, Direction);
}

bool UActionComponent::TryStartActionByTid(const int32 ActionTid)
{
	return TryStartActionByTid(ActionTid, EActionDirection::Any);
}

bool UActionComponent::TryStartActionByTid(const int32 ActionTid, const EActionDirection Direction)
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		LastStartResult = EActionStartResult::TableManagerUnavailable;
		return false;
	}

	const FActionDataRow* ActionData = TableManager->FindActionData(ActionTid);
	if (!ActionData)
	{
		LastStartResult = EActionStartResult::ActionDataNotFound;
		return false;
	}

	if (!CanStartAction(*ActionData, Direction))
	{
		return false;
	}

	BeginAction(*ActionData, Direction);
	return true;
}

void UActionComponent::CompleteCurrentAction()
{
	if (ActiveActionTid == ActionComponentInvalidActionTid)
	{
		return;
	}

	const int32 CompletedActionTid = ActiveActionTid;
	const EActionType CompletedActionType = ActiveActionType;

	ActiveActionTid = ActionComponentInvalidActionTid;
	ActiveActionType = EActionType::None;
	RuntimeState = EActionRuntimeState::None;
	ActiveActionDirection = EActionDirection::Any;
	bActiveActionInterruptLocked = false;
	bActiveActionInputBufferOpen = false;
	const bool bShouldResumeStaminaRecovery = bActiveActionPausedStaminaRecovery;
	bActiveActionPausedStaminaRecovery = false;
	ClearBufferedAction();

	if (bShouldResumeStaminaRecovery && CachedStatComponent)
	{
		CachedStatComponent->ResumeStaminaRecovery(ActionStaminaRecoveryPauseSource, true);
	}

	ClearActiveActionStaminaRecoveryRateMultiplier();

	OnActionCompleted.Broadcast(CompletedActionTid, CompletedActionType);
	RefreshTickEnabled();
}

bool UActionComponent::ConsumeActionStaminaCostByType(const EActionType ActionType, const float CostMultiplier)
{
	if (const FActionDataRow* ActiveActionData = GetActiveActionData();
		ActiveActionData && ActiveActionData->ActionType == ActionType)
	{
		return ConsumeStamina(*ActiveActionData, GetOnDemandStaminaCost(*ActiveActionData, CostMultiplier), false, true);
	}

	// GuardHit처럼 액션 인스턴스보다 판정 상태가 오래 유지되는 경우에도 같은 ActionData 비용 규칙을 사용한다.
	if (const FActionDataRow* ActionData = FindFirstActionDataByType(ActionType))
	{
		return ConsumeStamina(*ActionData, GetOnDemandStaminaCost(*ActionData, CostMultiplier), false, true);
	}

	return false;
}

void UActionComponent::ApplyActiveActionStaminaRecoveryRateMultiplier()
{
	if (const FActionDataRow* ActionData = GetActiveActionData())
	{
		ApplyStaminaRecoveryRateMultiplier(*ActionData);
	}
}

void UActionComponent::ClearActiveActionStaminaRecoveryRateMultiplier()
{
	if (!bActiveActionModifiedStaminaRecoveryRate)
	{
		return;
	}

	if (CachedStatComponent)
	{
		CachedStatComponent->ClearStaminaRecoveryRateMultiplier(ActionStaminaRecoveryRateMultiplierSource);
	}
	bActiveActionModifiedStaminaRecoveryRate = false;
}

void UActionComponent::CancelCurrentAction()
{
	CompleteCurrentAction();
	ClearBufferedAction();
}

void UActionComponent::SetActiveActionInterruptLocked(const bool bNewInterruptLocked)
{
	if (ActiveActionTid == ActionComponentInvalidActionTid)
	{
		bActiveActionInterruptLocked = false;
		return;
	}

	bActiveActionInterruptLocked = bNewInterruptLocked;
	if (!bActiveActionInterruptLocked)
	{
		TryStartBufferedAction();
	}
}

void UActionComponent::SetActiveActionInputBufferOpen(const bool bNewInputBufferOpen)
{
	if (ActiveActionTid == ActionComponentInvalidActionTid)
	{
		bActiveActionInputBufferOpen = false;
		ClearBufferedAction();
		return;
	}

	bActiveActionInputBufferOpen = bNewInputBufferOpen;
	if (!bActiveActionInputBufferOpen)
	{
		TryStartBufferedAction();
	}
}

void UActionComponent::UpdateBufferedActionDirection(const EActionDirection Direction)
{
	if (BufferedActionTid == ActionComponentInvalidActionTid)
	{
		return;
	}

	BufferedActionDirection = Direction;
}

void UActionComponent::SetCombatStance(const ECombatStance NewCombatStance)
{
	CombatStance = NewCombatStance;
}

void UActionComponent::SetGuardState(const EGuardState NewGuardState)
{
	GuardState = NewGuardState;
}

void UActionComponent::SetWeaponType(const EActionWeaponType NewWeaponType)
{
	WeaponType = NewWeaponType;
}

const FActionDataRow* UActionComponent::GetActiveActionData() const
{
	if (ActiveActionTid == ActionComponentInvalidActionTid)
	{
		return nullptr;
	}

	const UBATableManager* TableManager = UBATableManager::Get(this);
	return TableManager ? TableManager->FindActionData(ActiveActionTid) : nullptr;
}

bool UActionComponent::IsMovementLockedByAction() const
{
	const FActionDataRow* ActionData = GetActiveActionData();
	return ActionData && ActionData->bLocksMovement && bActiveActionInterruptLocked;
}

bool UActionComponent::IsActiveActionUsingRootMotion() const
{
	const FActionDataRow* ActionData = GetActiveActionData();
	return ActionData && ActionData->bUsesRootMotion;
}

const FMovesetRow* UActionComponent::FindBestMoveset(
	const EActionCommand Command,
	const EActionDirection Direction) const
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		return nullptr;
	}

	const FMovesetRow* BestRow = nullptr;
	int32 BestScore = MIN_int32;

	for (const TPair<int32, FMovesetRow*>& Pair : TableManager->GetMovesetTable())
	{
		const FMovesetRow* Row = Pair.Value;
		if (!Row || Row->Command != Command)
		{
			continue;
		}
		// Moveset 테이블의 MovesetKey를 현재 ActionComponent가 갖고 있어야 함
		if (!Row->MovesetKey.IsNone() && !MovesetKeys.Contains(Row->MovesetKey))
		{
			continue;
		}
		if (Row->CombatStance != CombatStance)
		{
			continue;
		}
		if (Row->GuardState != GuardState)
		{
			const FActionDataRow* RowActionData = TableManager->FindActionData(Row->ActionTid);
			// 가드 중 특수 행동은 별도 GuardState Moveset을 만들지 않아도 기본 Row를 재사용한다.
			// 이 예외가 없으면 CanStartAction까지 도달하기 전에 Moveset 검색 단계에서 막힌다.
			const bool bCanUseDefaultGuardStateForInterrupt =
				ActiveActionType == EActionType::Guard
				&& Row->GuardState == EGuardState::None
				&& RowActionData
				&& IsGuardInterruptActionType(RowActionData->ActionType);
			if (!bCanUseDefaultGuardStateForInterrupt)
			{
				continue;
			}
		}
		if (Row->WeaponType != EActionWeaponType::Any && Row->WeaponType != WeaponType)
		{
			continue;
		}
		if (Row->Direction != EActionDirection::Any && Row->Direction != Direction)
		{
			continue;
		}

		int32 Score = Row->Priority * 100;
		Score += MovesetKeys.Contains(Row->MovesetKey) ? 16 : 0;
		Score += Row->WeaponType == WeaponType ? 8 : 0;
		Score += Row->Direction == Direction ? 4 : 0;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestRow = Row;
		}
	}

	return BestRow;
}

bool UActionComponent::CanStartAction(const FActionDataRow& ActionData, const EActionDirection Direction)
{
	if (ActiveActionTid != ActionComponentInvalidActionTid)
	{
		if (bActiveActionInputBufferOpen && !bConsumingBufferedAction)
		{
			BufferAction(ActionData.Tid, Direction);
			LastStartResult = EActionStartResult::Buffered;
			return false;
		}

		const FActionDataRow* ActiveActionData = GetActiveActionData();
		const bool bCanInterruptActiveAction =
			ActiveActionData && ActiveActionData->bCanBeInterrupted && !bActiveActionInterruptLocked;
		// 가드는 입력 유지형 방어 액션이라 구르기, 공격, 스프린트 같은 특수 행동으로 즉시 빠져나갈 수 있어야 한다.
		// 데이터의 bCanBeInterrupted를 넓게 열면 모든 액션이 끼어들 수 있어 허용 타입만 예외로 둔다.
		const bool bCanActionInterruptGuard =
			ActiveActionType == EActionType::Guard
			&& IsGuardInterruptActionType(ActionData.ActionType)
			&& !bActiveActionInterruptLocked;
		if (!bCanInterruptActiveAction && !bCanActionInterruptGuard)
		{
			LastStartResult = EActionStartResult::AlreadyRunning;
			return false;
		}
	}

	if (ActiveActionTid != ActionData.Tid && IsActionOnCooldown(ActionData.Tid))
	{
		LastStartResult = EActionStartResult::Cooldown;
		return false;
	}

	if (CachedStatComponent)
	{
		if (!CanConsumeStamina(ActionData, GetStartStaminaCost(ActionData)))
		{
			LastStartResult = EActionStartResult::NotEnoughStamina;
			return false;
		}
	}

	return true;
}

void UActionComponent::BeginAction(const FActionDataRow& ActionData, const EActionDirection Direction)
{
	const bool bInterruptingGuard = ActiveActionType == EActionType::Guard
		&& ActionData.ActionType != EActionType::Guard;

	CompleteCurrentAction();
	if (bInterruptingGuard)
	{
		GuardState = EGuardState::None;
	}

	ActiveActionTid = ActionData.Tid;
	ActiveActionType = ActionData.ActionType;
	RuntimeState = GetRuntimeStateForAction(ActionData);
	ActiveActionDirection = Direction;
	bActiveActionInterruptLocked = false;
	bActiveActionInputBufferOpen = false;
	bActiveActionPausedStaminaRecovery = false;
	bActiveActionModifiedStaminaRecoveryRate = false;
	LastStartResult = EActionStartResult::Success;

	// TODO: 스탯 컴포넌트 결합 의존성 없애기 - Delegate로 디커플링 (곽민규)
	ConsumeStamina(ActionData, GetStartStaminaCost(ActionData), true, false);
	// 가드는 Start/End가 아니라 가드 윈도우(GuardWindow)가 열린 동안만 회복 배율을 적용한다.
	if (ActionData.ActionType != EActionType::Guard)
	{
		ApplyStaminaRecoveryRateMultiplier(ActionData);
	}
	StartCooldown(ActionData);

	OnActionStarted.Broadcast(ActiveActionTid, ActiveActionType);
	RefreshTickEnabled();
}

void UActionComponent::StartCooldown(const FActionDataRow& ActionData)
{
	if (ActionData.Cooldown > 0.f)
	{
		CooldownRemainingByActionTid.Add(ActionData.Tid, ActionData.Cooldown);
	}
}

void UActionComponent::ApplyStaminaRecoveryRateMultiplier(const FActionDataRow& ActionData)
{
	if (!CachedStatComponent
		|| bActiveActionModifiedStaminaRecoveryRate
		|| FMath::IsNearlyEqual(ActionData.StaminaRecoveryRateMultiplier, 1.f))
	{
		return;
	}

	CachedStatComponent->SetStaminaRecoveryRateMultiplier(
		ActionStaminaRecoveryRateMultiplierSource,
		ActionData.StaminaRecoveryRateMultiplier);
	bActiveActionModifiedStaminaRecoveryRate = true;
}

bool UActionComponent::CanConsumeStamina(
	const FActionDataRow& ActionData,
	const float StaminaCost) const
{
	if (!CachedStatComponent)
	{
		return true;
	}

	const float RequiredStamina = FMath::Max(
		ActionData.MinRequiredStamina,
		FMath::Max(0.f, StaminaCost));

	return CachedStatComponent->GetCurrentStamina() >= RequiredStamina;
}

bool UActionComponent::ConsumeStamina(
	const FActionDataRow& ActionData,
	const float StaminaCost,
	const bool bPauseRecovery,
	const bool bRestartRecoveryDelay)
{
	if (!CachedStatComponent)
	{
		return true;
	}

	const float SafeStaminaCost = FMath::Max(0.f, StaminaCost);
	if (SafeStaminaCost <= 0.f)
	{
		return true;
	}

	if (!CanConsumeStamina(ActionData, SafeStaminaCost))
	{
		LastStartResult = EActionStartResult::NotEnoughStamina;
		return false;
	}

	if (bPauseRecovery)
	{
		CachedStatComponent->PauseStaminaRecovery(ActionStaminaRecoveryPauseSource);
		bActiveActionPausedStaminaRecovery = true;
	}

	CachedStatComponent->ConsumeStamina(SafeStaminaCost);
	if (bRestartRecoveryDelay)
	{
		CachedStatComponent->RestartStaminaRecoveryDelay();
	}
	return true;
}

const FActionDataRow* UActionComponent::FindFirstActionDataByType(const EActionType ActionType) const
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (!TableManager)
	{
		return nullptr;
	}

	const FActionDataRow* BestActionData = nullptr;
	for (const TPair<int32, FActionDataRow*>& Pair : TableManager->GetActionDataTable())
	{
		const FActionDataRow* ActionData = Pair.Value;
		if (!ActionData || ActionData->ActionType != ActionType)
		{
			continue;
		}

		if (!BestActionData || ActionData->Tid < BestActionData->Tid)
		{
			BestActionData = ActionData;
		}
	}

	return BestActionData;
}

float UActionComponent::GetStartStaminaCost(
	const FActionDataRow& ActionData,
	const float CostMultiplier) const
{
	return ActionData.StaminaCostType == EActionStaminaCostType::Instant
		? FMath::Max(0.f, ActionData.StaminaCost) * FMath::Max(0.f, CostMultiplier)
		: 0.f;
}

float UActionComponent::GetOnDemandStaminaCost(
	const FActionDataRow& ActionData,
	const float CostMultiplier) const
{
	return ActionData.StaminaCostType == EActionStaminaCostType::OnDemand
		? FMath::Max(0.f, ActionData.StaminaCost) * FMath::Max(0.f, CostMultiplier)
		: 0.f;
}

void UActionComponent::BufferAction(const int32 ActionTid, const EActionDirection Direction)
{
	BufferedActionTid = ActionTid;
	BufferedActionDirection = Direction;
}

void UActionComponent::ClearBufferedAction()
{
	BufferedActionTid = ActionComponentInvalidActionTid;
	BufferedActionDirection = EActionDirection::Any;
}

void UActionComponent::TryStartBufferedAction()
{
	if (bConsumingBufferedAction
		|| BufferedActionTid == ActionComponentInvalidActionTid
		|| ActiveActionTid == ActionComponentInvalidActionTid
		|| bActiveActionInterruptLocked
		|| bActiveActionInputBufferOpen)
	{
		return;
	}

	const int32 ActionTidToStart = BufferedActionTid;
	EActionDirection DirectionToStart = BufferedActionDirection;
	if (ResolveBufferedActionDirection.IsBound())
	{
		DirectionToStart = ResolveBufferedActionDirection.Execute(ActionTidToStart, DirectionToStart);
	}
	ClearBufferedAction();

	TGuardValue<bool> ConsumingGuard(bConsumingBufferedAction, true);
	TryStartActionByTid(ActionTidToStart, DirectionToStart);
}

void UActionComponent::RefreshTickEnabled()
{
	SetComponentTickEnabled(!CooldownRemainingByActionTid.IsEmpty());
}

bool UActionComponent::IsActionOnCooldown(const int32 ActionTid) const
{
	const float* Remaining = CooldownRemainingByActionTid.Find(ActionTid);
	return Remaining && *Remaining > 0.f;
}

EActionRuntimeState UActionComponent::GetRuntimeStateForAction(const FActionDataRow& ActionData) const
{
	switch (ActionData.ActionType)
	{
	case EActionType::LightAttack:
	case EActionType::HeavyAttack:
		return EActionRuntimeState::Attacking;
	case EActionType::Guard:
		return EActionRuntimeState::Guarding;
	case EActionType::DodgeRoll:
	case EActionType::Backstep:
		return EActionRuntimeState::Dodging;
	case EActionType::UseConsumable:
		return EActionRuntimeState::UsingItem;
	case EActionType::None:
	case EActionType::Sprint:
	default:
		return EActionRuntimeState::None;
	}
}
