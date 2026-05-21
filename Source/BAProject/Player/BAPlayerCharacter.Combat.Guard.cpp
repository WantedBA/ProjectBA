#include "Player/BAPlayerCharacter.h"

#include "Component/ActionComponent.h"
#include "Component/ActionAnimationComponent.h"

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

	SetGuardWindowActive(false);
	SetPerfectGuardWindowActive(false);

	if (!ActionComponent->TryStartAction(EActionCommand::Guard))
	{
		return false;
	}

	if (ActionComponent->GetActiveActionType() != EActionType::Guard)
	{
		return false;
	}

	ConfigureGuardMontageSections();
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
		bStoppedGuard = true;
		break;
	case EGuardState::None:
	default:
		break;
	}
	SetGuardWindowActive(false);
	if (GuardState == EGuardState::GuardBroken)
	{
		ActionComponent->SetGuardState(EGuardState::None);
	}

	if (ActionComponent->GetActiveActionType() == EActionType::Guard)
	{
		bStoppedGuard = true;
		if (RequestGuardMontageEnd())
		{
			SetBAPlayerState(EBAPlayerState::None);
			SetCombatMode(EPlayerCombatMode::None);
			return;
		}

		ActionComponent->CompleteCurrentAction();
	}

	if (!bStoppedGuard || IsDamageReacting())
	{
		return;
	}

	SetBAPlayerState(EBAPlayerState::None);
	SetCombatMode(EPlayerCombatMode::None);
}

void ABAPlayerCharacter::ConfigureGuardMontageSections()
{
	if (!ActionAnimationComponent)
	{
		return;
	}

	ActionAnimationComponent->SetActiveMontageNextSection(GuardStartSection, GuardLoopSection);
	ActionAnimationComponent->SetActiveMontageNextSection(GuardLoopSection, GuardLoopSection);
}

bool ABAPlayerCharacter::RequestGuardMontageEnd()
{
	if (!ActionAnimationComponent)
	{
		return false;
	}

	ActionAnimationComponent->SetActiveMontageNextSection(GuardStartSection, GuardEndSection);
	ActionAnimationComponent->SetActiveMontageNextSection(GuardLoopSection, GuardEndSection);
	return ActionAnimationComponent->JumpActiveMontageToSection(GuardEndSection);
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

void ABAPlayerCharacter::SetGuardWindowActive(const bool bActive)
{
	if (!ActionComponent)
	{
		SetPerfectGuardWindowActive(false);
		return;
	}

	if (bActive)
	{
		if (ActionComponent->GetActiveActionType() == EActionType::Guard && !IsDamageReacting())
		{
			ActionComponent->SetGuardState(EGuardState::Guarding);
		}
		return;
	}

	const EGuardState GuardState = ActionComponent->GetGuardState();
	if (GuardState == EGuardState::Guarding || GuardState == EGuardState::Blocking)
	{
		ActionComponent->SetGuardState(EGuardState::None);
	}
	SetPerfectGuardWindowActive(false);
}

void ABAPlayerCharacter::SetPerfectGuardWindowActive(const bool bActive)
{
	if (bActive && (!ActionComponent
			|| (ActionComponent->GetGuardState() != EGuardState::Guarding
				&& ActionComponent->GetGuardState() != EGuardState::Blocking)))
	{
		return;
	}

	bPerfectGuardWindowActive = bActive;
}

bool ABAPlayerCharacter::IsPerfectGuardWindowActive() const
{
	return bPerfectGuardWindowActive;
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

void ABAPlayerCharacter::HandlePerfectGuardSucceeded(const FHitResult& HitResult, AActor* DamageCauser)
{
	ConsumePerfectGuardStaminaCost();
	K2_OnPerfectGuardSucceeded(HitResult, DamageCauser);

	if (ACharacterBase* DamageCauserCharacter = Cast<ACharacterBase>(DamageCauser))
	{
		DamageCauserCharacter->OnAttackPerfectGuarded.Broadcast(this, HitResult);
	}

	// 슬로우모션은 아직 합의되지 않은 스펙이라 플레이어 가드 성공 처리 쪽에 위치만 남기고 비활성화한다.
	// if (UWorld* World = GetWorld())
	// {
	// 	if (UBATimeSubsystem* TimeSubsystem = World->GetSubsystem<UBATimeSubsystem>())
	// 	{
	// 		TimeSubsystem->ApplySlowMotion(TimeDilation, Duration);
	// 	}
	// }
}
