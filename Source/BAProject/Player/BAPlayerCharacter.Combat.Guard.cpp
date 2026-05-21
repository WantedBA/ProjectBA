#include "Player/BAPlayerCharacter.h"

#include "Component/ActionComponent.h"

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
		ActionComponent->SetGuardState(EGuardState::None);
		bStoppedGuard = true;
		break;
	case EGuardState::GuardBroken:
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
