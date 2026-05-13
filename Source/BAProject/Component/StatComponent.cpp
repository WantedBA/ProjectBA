#include "Component/StatComponent.h"

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

	// 현재 스테미너가 꽉 찼거나 바닥났을 경우, 혹은 리젠 양이 0일 경우 타이머 초기화
	if (MaxStamina <= 0.f || StaminaRegenAmount <= 0.f || CurrentStamina >= MaxStamina)
	{
		StaminaRegenDelayRemaining = 0.f;
		SetComponentTickEnabled(false);
		return;
	}

	// 리젠 딜레이가 남아있을 경우 프레임 경과 시간만큼 딜레이 차감
	if (StaminaRegenDelayRemaining > 0.f)
	{
		StaminaRegenDelayRemaining = FMath::Max(0.f, StaminaRegenDelayRemaining - DeltaTime);
		return;
	}

	// 리젠 양에 프레임 경과 시간 곱한만큼 회복
	SetCurrentStamina(CurrentStamina + StaminaRegenAmount * DeltaTime);
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
	StaminaRegenAmount = InStaminaRegenAmount;
	StaminaRegenDelay = InStaminaRegenDelay;
	StaminaRegenDelayRemaining = 0.f;
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

	// 스테미너가 소모되고 있을 경우 타이머 최신화
	if (CurrentStamina < OldStamina)
	{
		StaminaRegenDelayRemaining = StaminaRegenDelay;
		SetComponentTickEnabled(CurrentStamina < MaxStamina && StaminaRegenAmount > 0.f);
		return;
	}

	// 스테미너 다 찼으면 타이머 초기화
	if (CurrentStamina >= MaxStamina)
	{
		StaminaRegenDelayRemaining = 0.f;
		SetComponentTickEnabled(false);
	}
}
