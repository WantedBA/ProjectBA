#include "Component/StatComponent.h"

namespace
{
	constexpr uint64 StaminaDebugMessageKey = 13020;
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

	// 현재 스테미너가 꽉 찼거나 바닥났을 경우, 혹은 회복률이 0일 경우 타이머 초기화
	if (MaxStamina <= 0.f || StaminaRecoveryPerSecond <= 0.f || CurrentStamina >= MaxStamina)
	{
		StaminaRecoveryDelayRemaining = 0.f;
		SetComponentTickEnabled(false);
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
	SetComponentTickEnabled(false);
	WalkSpeed = InWalkSpeed;
	RunSpeed = InRunSpeed;
	SprintSpeed = InSprintSpeed;
	Attack = InAttack;
	AttackSpeed = InAttackSpeed;
	Defence = InDefence;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
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

	// 스테미너가 소모되고 있을 경우 타이머 최신화
	if (CurrentStamina < OldStamina)
	{
		StaminaRecoveryDelayRemaining = StaminaRecoveryDelay;
		SetComponentTickEnabled(CurrentStamina < MaxStamina && StaminaRecoveryPerSecond > 0.f);

		return;
	}

	// 스테미너 다 찼으면 타이머 초기화
	if (CurrentStamina >= MaxStamina)
	{
		StaminaRecoveryDelayRemaining = 0.f;
		SetComponentTickEnabled(false);
	}
}
