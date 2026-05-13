#include "Component/StatComponent.h"

UStatComponent::UStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentHP = MaxHP;
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
	const float InStaminaRegenAmount,
	const float InStaminaRegenDelay,
	const float InMoveSpeed,
	const float InAttack,
	const float InAttackSpeed,
	const float InDefence
)
{
	MaxHP = InMaxHP;
	CurrentHP = MaxHP;
	MaxStamina = InMaxStamina;
	CurrentStamina = MaxStamina;
	StaminaRegenAmount = InStaminaRegenAmount;
	StaminaRegenDelay = InStaminaRegenDelay;
	MoveSpeed = InMoveSpeed;
	Attack = InAttack;
	AttackSpeed = InAttackSpeed;
	Defence = InDefence;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}
