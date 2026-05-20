#include "Component/StatComponent.h"

namespace
{
	const FName DefaultStaminaRecoveryPauseSource(TEXT("Default"));
}

UStatComponent::UStatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	CurrentHP = MaxHP;
}

void UStatComponent::TickComponent
(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 현재 스테미너가 꽉 찼거나 회복률이 0일 경우 타이머 초기화
	if (MaxStamina <= 0.f || StaminaRecoveryPerSecond <= 0.f || CurrentStamina >= MaxStamina)
	{
		StaminaRecoveryDelayRemaining = 0.f;
		RefreshStaminaRecoveryTick();
		return;
	}

	if (IsStaminaRecoveryPaused())
	{
		RefreshStaminaRecoveryTick();
		return;
	}

	// 리젠 딜레이가 남아있을 경우 프레임 경과 시간만큼 딜레이 차감
	if (StaminaRecoveryDelayRemaining > 0.f)
	{
		StaminaRecoveryDelayRemaining = FMath::Max(0.f, StaminaRecoveryDelayRemaining - DeltaTime);
		return;
	}

	// 초당 회복률을 실제 초당 회복량으로 바꾼 뒤, 이번 프레임 시간만큼만 회복
	const float RecoveryAmountPerSecond = MaxStamina * StaminaRecoveryPerSecond / 100.f;
	const float RecoveryAmountThisFrame = RecoveryAmountPerSecond * DeltaTime;
	SetCurrentStamina(CurrentStamina + RecoveryAmountThisFrame);
}

void UStatComponent::RestoreAll()
{
	CurrentHP = MaxHP;
	CurrentStamina = MaxStamina;
	StaminaRecoveryDelayRemaining = 0.f;
	StaminaRecoveryPauseSources.Reset();

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	RefreshStaminaRecoveryTick();
}

void UStatComponent::ApplyDamage(float DamageAmount)
{
	if (IsDead())
	{
		return;
	}

	float FinalDamage = FMath::Max(0.f, DamageAmount - Defence);
	float OldHP = CurrentHP;
	CurrentHP = FMath::Clamp(CurrentHP - FinalDamage, 0.f, MaxHP);

	if (OldHP != CurrentHP)
	{
		OnHPChanged.Broadcast(CurrentHP, MaxHP);
	}

	if (CurrentHP <= 0.f)
	{
		OnDead.Broadcast();
	}
}

void UStatComponent::InitializeStats(float InMaxHP, float InAttack, float InDefence)
{
	MaxHP = InMaxHP;
	Attack = InAttack;
	Defence = InDefence;
	CurrentHP = MaxHP;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

void UStatComponent::InitializeStats
(
	const float InMaxHP,
	const float InMaxStamina,
	const float InStaminaRecoveryPerSecond,
	const float InStaminaRecoveryDelay,
	const float InWalkSpeed,
	const float InRunSpeed,
	const float InSprintSpeed,
	const float InAttack,
	const float InAttackSpeed,
	const float InDefence
)
{
	MaxHP = InMaxHP;
	CurrentHP = MaxHP;
	MaxStamina = InMaxStamina;
	CurrentStamina = MaxStamina;
	StaminaRecoveryPerSecond = InStaminaRecoveryPerSecond;
	StaminaRecoveryDelay = InStaminaRecoveryDelay;
	StaminaRecoveryDelayRemaining = 0.f;
	StaminaRecoveryPauseSources.Reset();
	RefreshStaminaRecoveryTick();
	WalkSpeed = InWalkSpeed;
	RunSpeed = InRunSpeed;
	SprintSpeed = InSprintSpeed;
	Attack = InAttack;
	AttackSpeed = InAttackSpeed;
	Defence = InDefence;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

void UStatComponent::ConsumeStamina(const float ConsumeAmount)
{
	if (ConsumeAmount <= 0.f)
	{
		return;
	}

	SetCurrentStamina(CurrentStamina - ConsumeAmount);
	RefreshStaminaRecoveryTick();
}

void UStatComponent::PauseStaminaRecovery(const FName Source)
{
	const FName SafeSource = Source.IsNone() ? DefaultStaminaRecoveryPauseSource : Source;
	StaminaRecoveryPauseSources.Add(SafeSource);
	RefreshStaminaRecoveryTick();
}

void UStatComponent::ResumeStaminaRecovery(const FName Source, const bool bApplyDelay)
{
	const FName SafeSource = Source.IsNone() ? DefaultStaminaRecoveryPauseSource : Source;
	const int32 RemovedCount = StaminaRecoveryPauseSources.Remove(SafeSource);
	if (RemovedCount <= 0)
	{
		RefreshStaminaRecoveryTick();
		return;
	}

	if (!IsStaminaRecoveryPaused() && bApplyDelay && CurrentStamina < MaxStamina)
	{
		StaminaRecoveryDelayRemaining = StaminaRecoveryDelay;
	}

	RefreshStaminaRecoveryTick();
}

void UStatComponent::SetCurrentStamina(const float NewCurrentStamina)
{
	const float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Clamp(NewCurrentStamina, 0.f, MaxStamina);

	// 스테미너 수치가 변경된 경우
	if (OldStamina != CurrentStamina)
	{
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	}

	// 스테미너 다 찼으면 타이머 초기화
	if (CurrentStamina >= MaxStamina)
	{
		StaminaRecoveryDelayRemaining = 0.f;
	}

	RefreshStaminaRecoveryTick();
}

void UStatComponent::RefreshStaminaRecoveryTick()
{
	const bool bCanRecover =
		MaxStamina > 0.f
		&& StaminaRecoveryPerSecond > 0.f
		&& CurrentStamina < MaxStamina
		&& !IsStaminaRecoveryPaused();

	SetComponentTickEnabled(bCanRecover);
}
