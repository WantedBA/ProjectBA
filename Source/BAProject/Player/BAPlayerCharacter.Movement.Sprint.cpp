#include "Player/BAPlayerCharacter.h"

#include "Component/StatComponent.h"

// 탈진으로 인해 Sprint 재진입이 잠겨 있는지 반환한다.
bool ABAPlayerCharacter::IsSprintLockedAfterExhausted() const
{
	return SprintRuntime.bLockedAfterExhausted;
}

// Sprint 가능 여부는 데이터 존재, 현재 스태미나, 탈진 후 재시작 조건을 함께 검사한다.
bool ABAPlayerCharacter::IsSprintAllowedByStamina() const
{
	if (!SprintCostSettings.bHasActionData || !StatComponent)
	{
		return false;
	}

	const float CurrentStamina = StatComponent->GetCurrentStamina();
	const float SprintRestartStamina = StatComponent->GetMaxStamina() * SprintCostSettings.RestartStaminaPercent / 100.f;

	if (SprintRuntime.bLockedAfterExhausted && CurrentStamina < SprintRestartStamina)
	{
		return false;
	}

	return CurrentStamina >= SprintCostSettings.MinRequiredStamina
		&& (SprintCostSettings.StaminaCost <= 0.f || CurrentStamina > 0.f);
}

// Sprint Loop 중에만 Action 데이터의 스태미나 비용 규칙에 따라 스태미나를 소모한다.
void ABAPlayerCharacter::DrainSprintStaminaDuringLoop(const float DeltaTime)
{
	if (MovementRuntime.ActiveGait != EMovementState::Sprint
		|| MovementRuntime.Phase != EPlayerMovementPhase::Loop
		|| !MovementRuntime.bHasMoveInput
		|| !StatComponent
		|| SprintCostSettings.StaminaCost <= 0.f)
	{
		return;
	}

	const float CurrentStamina = StatComponent->GetCurrentStamina();
	const float ConsumeAmount = CalculateSprintStaminaDrain(DeltaTime);
	StatComponent->SetCurrentStamina(CurrentStamina - ConsumeAmount);

	LockSprintUntilRecovered();
}

// Action 데이터의 비용 타입에 따라 이번 프레임 Sprint 스태미나 소모량을 계산한다.
float ABAPlayerCharacter::CalculateSprintStaminaDrain(const float DeltaTime) const
{
	if (!StatComponent)
	{
		return 0.f;
	}

	switch (SprintCostSettings.StaminaCostType)
	{
	case EPlayerStaminaCostType::PerSecond:
		return StatComponent->GetMaxStamina() * SprintCostSettings.StaminaCost / 100.f * DeltaTime;
	case EPlayerStaminaCostType::Instant:
	default:
		return SprintCostSettings.StaminaCost;
	}
}

// 스태미나가 0 이하가 되면 Sprint 재진입을 잠근다.
void ABAPlayerCharacter::LockSprintUntilRecovered()
{
	if (!StatComponent || SprintCostSettings.StaminaCost <= 0.f)
	{
		return;
	}

	if (StatComponent->GetCurrentStamina() <= 0.f)
	{
		SprintRuntime.bLockedAfterExhausted = true;
	}
}

// 탈진 락이 걸린 상태에서 재시작 기준 스태미나까지 회복됐는지 검사한다.
void ABAPlayerCharacter::UnlockSprintAfterRecovery()
{
	if (!SprintRuntime.bLockedAfterExhausted || !StatComponent)
	{
		return;
	}

	const float SprintRestartStamina = StatComponent->GetMaxStamina() * SprintCostSettings.RestartStaminaPercent / 100.f;
	if (StatComponent->GetCurrentStamina() >= SprintRestartStamina)
	{
		SprintRuntime.bLockedAfterExhausted = false;
	}
}
