#include "Component/StatComponent.h"

namespace
{
	const FName DefaultStaminaRecoveryPauseSource(TEXT("Default"));
	const FName DefaultStaminaRecoveryRateMultiplierSource(TEXT("Default"));
}

UStatComponent::UStatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	CurrentHP = MaxHP;
}

void UStatComponent::ResetModifiers()
{
	AttackSpeedModifier = 1.f;
	MaxStaminaModifier = 1.f;
	GuardDamageReductionRateModifier = 1.f;
	StaminaRecoveryModifier = 1.f;
}

void UStatComponent::SetAttackSpeedModifier(const float NewModifier)
{
	AttackSpeedModifier = NewModifier;
}

void UStatComponent::SetMaxStaminaModifier(const float NewModifier)
{
	MaxStaminaModifier = NewModifier;
}

void UStatComponent::SetGuardDamageReductionRateModifier(const float NewModifier)
{
	GuardDamageReductionRateModifier = NewModifier;
}

void UStatComponent::SetStaminaRecoveryModifier(const float NewModifier)
{
	StaminaRecoveryModifier = NewModifier;
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
	if (GetMaxStamina() <= 0.f || StaminaRecoveryPerSecond <= 0.f || CurrentStamina >= GetMaxStamina())
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

	// 초당 회복률을 실제 초당 회복량으로 바꾼 뒤, 이번 프레임 시간과 회복 배율만큼만 회복
	const float RecoveryAmountPerSecond = GetMaxStamina() * StaminaRecoveryPerSecond * StaminaRecoveryModifier / 100.f;
	const float RecoveryAmountThisFrame = RecoveryAmountPerSecond * GetStaminaRecoveryRateMultiplier() * DeltaTime;
	SetCurrentStamina(CurrentStamina + RecoveryAmountThisFrame);
}

void UStatComponent::RestoreAll()
{
	CurrentHP = MaxHP;
	CurrentStamina = GetMaxStamina();
	StaminaRecoveryDelayRemaining = 0.f;
	StaminaRecoveryPauseSources.Reset();
	StaminaRecoveryRateMultipliers.Reset();

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, GetMaxStamina());

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
	CurrentStamina = GetMaxStamina();
	StaminaRecoveryPerSecond = InStaminaRecoveryPerSecond;
	StaminaRecoveryDelay = InStaminaRecoveryDelay;
	StaminaRecoveryDelayRemaining = 0.f;
	StaminaRecoveryPauseSources.Reset();
	StaminaRecoveryRateMultipliers.Reset();
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

void UStatComponent::RestartStaminaRecoveryDelay()
{
	if (CurrentStamina < MaxStamina)
	{
		StaminaRecoveryDelayRemaining = StaminaRecoveryDelay;
	}

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

	if (!IsStaminaRecoveryPaused() && bApplyDelay && CurrentStamina < GetMaxStamina())
	{
		StaminaRecoveryDelayRemaining = StaminaRecoveryDelay;
	}

	RefreshStaminaRecoveryTick();
}

void UStatComponent::SetStaminaRecoveryRateMultiplier(const FName Source, const float Multiplier)
{
	const FName SafeSource = Source.IsNone() ? DefaultStaminaRecoveryRateMultiplierSource : Source;
	StaminaRecoveryRateMultipliers.Add(SafeSource, FMath::Max(0.f, Multiplier));
	RefreshStaminaRecoveryTick();
}

void UStatComponent::ClearStaminaRecoveryRateMultiplier(const FName Source)
{
	const FName SafeSource = Source.IsNone() ? DefaultStaminaRecoveryRateMultiplierSource : Source;
	StaminaRecoveryRateMultipliers.Remove(SafeSource);
	RefreshStaminaRecoveryTick();
}

float UStatComponent::GetStaminaRecoveryRateMultiplier() const
{
	float Multiplier = 1.f;
	for (const TPair<FName, float>& Pair : StaminaRecoveryRateMultipliers)
	{
		Multiplier *= FMath::Max(0.f, Pair.Value);
	}

	return Multiplier;
}

void UStatComponent::SetCurrentStamina(const float NewCurrentStamina)
{
	const float OldStamina = CurrentStamina;
	CurrentStamina = FMath::Clamp(NewCurrentStamina, 0.f, GetMaxStamina());

	// 스테미너 수치가 변경된 경우
	if (OldStamina != CurrentStamina)
	{
		OnStaminaChanged.Broadcast(CurrentStamina, GetMaxStamina());
	}

	// 스테미너 다 찼으면 타이머 초기화
	if (CurrentStamina >= GetMaxStamina())
	{
		StaminaRecoveryDelayRemaining = 0.f;
	}

	RefreshStaminaRecoveryTick();
}

void UStatComponent::Heal()
{
	if (HealCount <= 0)
	{
		return;
	}
	
	if (IsDead())
	{
		return;
	}

	float OldHP = CurrentHP;
	CurrentHP = FMath::Clamp(CurrentHP + GetHealAmount(), 0.f, GetMaxHP());

	if (OldHP != CurrentHP)
	{
		OnHPChanged.Broadcast(CurrentHP, MaxHP);
	}
}

void UStatComponent::RefreshStaminaRecoveryTick()
{
	const bool bCanRecover =
		GetMaxStamina() > 0.f
		&& StaminaRecoveryPerSecond > 0.f
		&& CurrentStamina < GetMaxStamina()
		&& !IsStaminaRecoveryPaused()
		&& GetStaminaRecoveryRateMultiplier() > 0.f;

	SetComponentTickEnabled(bCanRecover);
}
