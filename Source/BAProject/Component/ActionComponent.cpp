// Copyright TeamBA. All Rights Reserved.

#include "Component/ActionComponent.h"

#include "Component/StatComponent.h"
#include "Tables/BATableManager.h"

namespace
{
	constexpr int32 ActionComponentInvalidActionTid = 0;
	const FName ActionStaminaRecoveryPauseSource(TEXT("Action"));
	const FName ActionStaminaRecoveryRateMultiplierSource(TEXT("Action"));
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
	const bool bShouldClearStaminaRecoveryRateMultiplier = bActiveActionModifiedStaminaRecoveryRate;
	bActiveActionPausedStaminaRecovery = false;
	bActiveActionModifiedStaminaRecoveryRate = false;
	ClearBufferedAction();

	if (bShouldResumeStaminaRecovery && CachedStatComponent)
	{
		CachedStatComponent->ResumeStaminaRecovery(ActionStaminaRecoveryPauseSource, true);
	}

	if (bShouldClearStaminaRecoveryRateMultiplier && CachedStatComponent)
	{
		CachedStatComponent->ClearStaminaRecoveryRateMultiplier(ActionStaminaRecoveryRateMultiplierSource);
	}

	OnActionCompleted.Broadcast(CompletedActionTid, CompletedActionType);
	RefreshTickEnabled();
}

bool UActionComponent::ConsumeActiveActionStaminaCost()
{
	const FActionDataRow* ActionData = GetActiveActionData();
	return ActionData && ConsumeStamina(*ActionData, EActionStaminaConsumeContext::OnDemand);
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
		if (Row->CombatStance != CombatStance || Row->GuardState != GuardState)
		{
			continue;
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
		if (!bCanInterruptActiveAction)
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
		if (!CanConsumeStamina(ActionData, EActionStaminaConsumeContext::Start))
		{
			LastStartResult = EActionStartResult::NotEnoughStamina;
			return false;
		}
	}

	return true;
}

void UActionComponent::BeginAction(const FActionDataRow& ActionData, const EActionDirection Direction)
{
	CompleteCurrentAction();

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
	ConsumeStamina(ActionData, EActionStaminaConsumeContext::Start);
	ApplyStaminaRecoveryRateMultiplier(ActionData);
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
	if (!CachedStatComponent || FMath::IsNearlyEqual(ActionData.StaminaRecoveryRateMultiplier, 1.f))
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
	const EActionStaminaConsumeContext ConsumeContext) const
{
	if (!CachedStatComponent)
	{
		return true;
	}

	const float RequiredStamina = FMath::Max(
		ActionData.MinRequiredStamina,
		GetStaminaCostForContext(ActionData, ConsumeContext));

	return CachedStatComponent->GetCurrentStamina() >= RequiredStamina;
}

bool UActionComponent::ConsumeStamina(
	const FActionDataRow& ActionData,
	const EActionStaminaConsumeContext ConsumeContext)
{
	if (!CachedStatComponent)
	{
		return true;
	}

	const float StaminaCost = GetStaminaCostForContext(ActionData, ConsumeContext);
	if (StaminaCost <= 0.f)
	{
		return true;
	}

	if (!CanConsumeStamina(ActionData, ConsumeContext))
	{
		LastStartResult = EActionStartResult::NotEnoughStamina;
		return false;
	}

	if (ConsumeContext == EActionStaminaConsumeContext::Start)
	{
		CachedStatComponent->PauseStaminaRecovery(ActionStaminaRecoveryPauseSource);
		bActiveActionPausedStaminaRecovery = true;
	}

	CachedStatComponent->ConsumeStamina(StaminaCost);
	return true;
}

float UActionComponent::GetStaminaCostForContext(
	const FActionDataRow& ActionData,
	const EActionStaminaConsumeContext ConsumeContext) const
{
	switch (ActionData.StaminaCostType)
	{
	case EActionStaminaCostType::Instant:
		return ConsumeContext == EActionStaminaConsumeContext::Start
			? FMath::Max(0.f, ActionData.StaminaCost)
			: 0.f;
	case EActionStaminaCostType::OnDemand:
		return ConsumeContext == EActionStaminaConsumeContext::OnDemand
			? FMath::Max(0.f, ActionData.StaminaCost)
			: 0.f;
	case EActionStaminaCostType::PerSecond:
	default:
		return 0.f;
	}
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
	const EActionDirection DirectionToStart = BufferedActionDirection;
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
