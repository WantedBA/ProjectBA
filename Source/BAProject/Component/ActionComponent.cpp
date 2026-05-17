// Copyright TeamBA. All Rights Reserved.

#include "Component/ActionComponent.h"

#include "Component/StatComponent.h"
#include "Tables/BATableManager.h"

namespace
{
	constexpr int32 InvalidActionTid = 0;
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

	return TryStartActionByTid(Moveset->ActionTid);
}

bool UActionComponent::TryStartActionByTid(const int32 ActionTid)
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

	if (!CanStartAction(*ActionData))
	{
		return false;
	}

	BeginAction(*ActionData);
	return true;
}

void UActionComponent::CompleteCurrentAction()
{
	if (ActiveActionTid == InvalidActionTid)
	{
		return;
	}

	const int32 CompletedActionTid = ActiveActionTid;
	const EActionType CompletedActionType = ActiveActionType;

	ActiveActionTid = InvalidActionTid;
	ActiveActionType = EActionType::None;
	RuntimeState = EActionRuntimeState::None;

	OnActionCompleted.Broadcast(CompletedActionTid, CompletedActionType);
	RefreshTickEnabled();
}

void UActionComponent::SetMovesetKey(const FName NewMovesetKey)
{
	MovesetKey = NewMovesetKey.IsNone() ? FName(TEXT("Default")) : NewMovesetKey;
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
	if (ActiveActionTid == InvalidActionTid)
	{
		return nullptr;
	}

	const UBATableManager* TableManager = UBATableManager::Get(this);
	return TableManager ? TableManager->FindActionData(ActiveActionTid) : nullptr;
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
		if (!Row->MovesetKey.IsNone() && Row->MovesetKey != MovesetKey)
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
		Score += Row->MovesetKey == MovesetKey ? 16 : 0;
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

bool UActionComponent::CanStartAction(const FActionDataRow& ActionData)
{
	if (ActiveActionTid != InvalidActionTid)
	{
		const FActionDataRow* ActiveActionData = GetActiveActionData();
		if (ActiveActionData && !ActiveActionData->bCanBeInterrupted)
		{
			LastStartResult = EActionStartResult::AlreadyRunning;
			return false;
		}
	}

	if (IsActionOnCooldown(ActionData.Tid))
	{
		LastStartResult = EActionStartResult::Cooldown;
		return false;
	}

	if (CachedStatComponent)
	{
		const float CurrentStamina = CachedStatComponent->GetCurrentStamina();
		if (CurrentStamina < ActionData.MinRequiredStamina || CurrentStamina < ActionData.StaminaCost)
		{
			LastStartResult = EActionStartResult::NotEnoughStamina;
			return false;
		}
	}

	return true;
}

void UActionComponent::BeginAction(const FActionDataRow& ActionData)
{
	CompleteCurrentAction();

	ActiveActionTid = ActionData.Tid;
	ActiveActionType = ActionData.ActionType;
	RuntimeState = GetRuntimeStateForAction(ActionData);
	LastStartResult = EActionStartResult::Success;

	ConsumeInstantCost(ActionData);
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

void UActionComponent::ConsumeInstantCost(const FActionDataRow& ActionData) const
{
	if (!CachedStatComponent
		|| ActionData.StaminaCost <= 0.f
		|| ActionData.StaminaCostType != EActionStaminaCostType::Instant)
	{
		return;
	}

	CachedStatComponent->SetCurrentStamina(CachedStatComponent->GetCurrentStamina() - ActionData.StaminaCost);
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
