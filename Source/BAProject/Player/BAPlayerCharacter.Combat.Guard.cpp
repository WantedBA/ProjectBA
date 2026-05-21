#include "Player/BAPlayerCharacter.h"

#include "Component/ActionComponent.h"

namespace
{
	// TODO: 무기 테이블로 분리 필요. 현재는 임시로 일반 가드/가드브레이크 피해 흡수율을 플레이어 코드에 고정한다.
	constexpr float GuardAbsorptionMultiplier = 0.5f;

	// TODO: 무기 테이블로 분리 필요. 퍼펙트 가드 스태미너 비용 배율도 장비별 정책으로 옮겨야 한다.
	constexpr float PerfectGuardStaminaCostMultiplier = 0.5f;
}

bool ABAPlayerCharacter::TryStartGuard()
{
	if (!CanAcceptActionInput() || !ActionComponent)
	{
		return false;
	}

	if (!ActionComponent->TryStartAction(EActionCommand::Guard))
	{
		return false;
	}

	if (ActionComponent->GetActiveActionType() != EActionType::Guard)
	{
		return false;
	}

	ActionComponent->SetGuardState(EGuardState::Guarding);
	SetBAPlayerState(EBAPlayerState::Guarding);
	SetCombatMode(EPlayerCombatMode::Block);
	return true;
}

void ABAPlayerCharacter::StopGuard()
{
	if (!ActionComponent)
	{
		return;
	}

	bool bStoppedGuard = false;
	const EGuardState GuardState = ActionComponent->GetGuardState();
	switch (GuardState)
	{
	case EGuardState::Guarding:
	case EGuardState::Blocking:
	case EGuardState::GuardBroken:
		ActionComponent->SetGuardState(EGuardState::None);
		bStoppedGuard = true;
		break;
	case EGuardState::None:
	default:
		break;
	}

	if (ActionComponent->GetActiveActionType() == EActionType::Guard)
	{
		ActionComponent->CompleteCurrentAction();
		bStoppedGuard = true;
	}

	if (!bStoppedGuard || IsDamageReacting())
	{
		return;
	}

	SetBAPlayerState(EBAPlayerState::None);
	SetCombatMode(EPlayerCombatMode::None);
}

void ABAPlayerCharacter::ConsumePerfectGuardStaminaCost()
{
	if (!ActionComponent)
	{
		return;
	}

	// TODO: 무기 테이블로 분리 필요. 퍼펙트 가드 스태미너 비용 배율 적용도 장비 정책에서 계산해야 한다.
	ActionComponent->ConsumeActiveActionStaminaCost(GetPerfectGuardStaminaCostMultiplier());
}

float ABAPlayerCharacter::GetGuardAbsorptionMultiplier() const
{
	return GuardAbsorptionMultiplier;
}

float ABAPlayerCharacter::GetPerfectGuardStaminaCostMultiplier() const
{
	return PerfectGuardStaminaCostMultiplier;
}

bool ABAPlayerCharacter::ConsumeGuardStaminaForDamage()
{
	if (!ActionComponent)
	{
		return false;
	}

	const bool bConsumedStamina = ActionComponent->ConsumeActiveActionStaminaCost();
	ActionComponent->SetGuardState(bConsumedStamina ? EGuardState::Blocking : EGuardState::GuardBroken);
	return bConsumedStamina;
}
