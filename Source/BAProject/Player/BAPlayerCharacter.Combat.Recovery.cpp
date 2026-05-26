#include "Player/BAPlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Component/ActionComponent.h"
#include "Component/PlayerWeaponVFX.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

void ABAPlayerCharacter::OpenRecoveryEscapeWindow(const FBAPlayerRecoveryEscapeWindowSettings& Settings)
{
	++RecoveryEscapeWindowCount;
	if (Settings.bAllowAttack)
	{
		++RecoveryEscapeAttackWindowCount;
	}
	if (Settings.bAllowDodge)
	{
		++RecoveryEscapeDodgeWindowCount;
	}
	if (Settings.bAllowGuard)
	{
		++RecoveryEscapeGuardWindowCount;
	}
	if (Settings.bAllowMove)
	{
		++RecoveryEscapeMoveWindowCount;
	}
}

void ABAPlayerCharacter::CloseRecoveryEscapeWindow(const FBAPlayerRecoveryEscapeWindowSettings& Settings)
{
	RecoveryEscapeWindowCount = FMath::Max(0, RecoveryEscapeWindowCount - 1);
	if (Settings.bAllowAttack)
	{
		RecoveryEscapeAttackWindowCount = FMath::Max(0, RecoveryEscapeAttackWindowCount - 1);
	}
	if (Settings.bAllowDodge)
	{
		RecoveryEscapeDodgeWindowCount = FMath::Max(0, RecoveryEscapeDodgeWindowCount - 1);
	}
	if (Settings.bAllowGuard)
	{
		RecoveryEscapeGuardWindowCount = FMath::Max(0, RecoveryEscapeGuardWindowCount - 1);
	}
	if (Settings.bAllowMove)
	{
		RecoveryEscapeMoveWindowCount = FMath::Max(0, RecoveryEscapeMoveWindowCount - 1);
	}
}

bool ABAPlayerCharacter::IsRecoveryEscapeWindowOpen() const
{
	return RecoveryEscapeWindowCount > 0;
}

bool ABAPlayerCharacter::IsRecoveryEscapeRequiredForCurrentState() const
{
	return IsAlive() && (IsAttackRecoveryEscapeState() || IsDamageReactionRecoveryEscapeState());
}

bool ABAPlayerCharacter::TryStartRecoveryEscapeAction(
	const EActionCommand Command,
	const EActionDirection Direction)
{
	if (!IsRecoveryEscapeRequiredForCurrentState())
	{
		return false;
	}

	if (Command == EActionCommand::LightAttack || Command == EActionCommand::HeavyAttack)
	{
		return TryStartAttackFromRecoveryEscape(Command);
	}

	if (Command == EActionCommand::Dodge)
	{
		if (!CanUseRecoveryEscapeDodge())
		{
			return false;
		}

		ExitCurrentRecoveryForEscape(false);
		if (!ActionComponent)
		{
			return false;
		}

		return Direction == EActionDirection::Any
			? ActionComponent->TryStartAction(EActionCommand::Dodge, Direction)
			: ActionComponent->TryStartActionOfType(EActionCommand::Dodge, Direction, EActionType::DodgeRoll);
	}

	if (Command == EActionCommand::Guard)
	{
		if (!CanUseRecoveryEscapeGuard())
		{
			return false;
		}

		ExitCurrentRecoveryForEscape(false);
		return TryStartGuard();
	}

	return false;
}

bool ABAPlayerCharacter::TryStartRecoveryEscapeMove(const FVector2D& MoveInput)
{
	if (!IsRecoveryEscapeRequiredForCurrentState())
	{
		return false;
	}

	if (MoveInput.IsNearlyZero())
	{
		return false;
	}

	// 공격 회복 중 이동 입력은 탈출 윈도우와 별개로 다음 공격 방향만 갱신한다.
	if (IsAttackRecoveryEscapeState())
	{
		SetMoveInputVector(MoveInput);
		SnapInterpolatedMoveInputTo(MoveInput);
		return true;
	}

	if (!CanUseRecoveryEscapeMove())
	{
		return false;
	}

	ExitCurrentRecoveryForEscape(false);
	SetMoveInputVector(MoveInput);
	SnapInterpolatedMoveInputTo(MoveInput);
	BeginMovementPhase(IsPhaseEnabledForGait(EPlayerMovementPhase::Start, MovementRuntime.ActiveGait)
		? EPlayerMovementPhase::Start
		: EPlayerMovementPhase::Loop);
	return true;
}

bool ABAPlayerCharacter::TryStartAttackFromRecoveryEscape(const EActionCommand Command)
{
	if (!CanUseRecoveryEscapeAttack())
	{
		return false;
	}

	if (IsAttackRecoveryEscapeState())
	{
		SetNextCombo(Command);
		if (!NextAttackMontage)
		{
			return false;
		}

		FaceMoveInputDirection();
		ExitAttackRecoveryForEscape(true);
		StartAttack(NextAttackMontage);
		return true;
	}

	if (IsDamageReactionRecoveryEscapeState())
	{
		NowComboTransitionTid = 0;
		SetNextCombo(Command);
		if (!NextAttackMontage)
		{
			return false;
		}

		FaceMoveInputDirection();
		ExitDamageReactionRecoveryForEscape();
		StartAttack(NextAttackMontage);
		return true;
	}

	return false;
}

bool ABAPlayerCharacter::IsAttackRecoveryEscapeState() const
{
	return BAPlayerState == EBAPlayerState::Attacking && ActiveAttackMontage != nullptr;
}

bool ABAPlayerCharacter::IsDamageReactionRecoveryEscapeState() const
{
	return BAPlayerState == EBAPlayerState::HitReacting
		&& (DamageReactionState == EPlayerDamageReactionState::HitReact
			|| DamageReactionState == EPlayerDamageReactionState::LargeHitReact);
}

bool ABAPlayerCharacter::CanUseRecoveryEscapeAttack() const
{
	return IsRecoveryEscapeRequiredForCurrentState()
		&& IsRecoveryEscapeWindowOpen()
		&& RecoveryEscapeAttackWindowCount > 0;
}

bool ABAPlayerCharacter::CanUseRecoveryEscapeDodge() const
{
	return IsRecoveryEscapeRequiredForCurrentState()
		&& IsRecoveryEscapeWindowOpen()
		&& RecoveryEscapeDodgeWindowCount > 0;
}

bool ABAPlayerCharacter::CanUseRecoveryEscapeGuard() const
{
	return IsRecoveryEscapeRequiredForCurrentState()
		&& IsRecoveryEscapeWindowOpen()
		&& RecoveryEscapeGuardWindowCount > 0;
}

bool ABAPlayerCharacter::CanUseRecoveryEscapeMove() const
{
	return IsRecoveryEscapeRequiredForCurrentState()
		&& IsRecoveryEscapeWindowOpen()
		&& RecoveryEscapeMoveWindowCount > 0;
}

void ABAPlayerCharacter::ClearRecoveryEscapeWindow()
{
	RecoveryEscapeWindowCount = 0;
	RecoveryEscapeAttackWindowCount = 0;
	RecoveryEscapeDodgeWindowCount = 0;
	RecoveryEscapeGuardWindowCount = 0;
	RecoveryEscapeMoveWindowCount = 0;
}

void ABAPlayerCharacter::ExitCurrentRecoveryForEscape(const bool bKeepQueuedAttack)
{
	if (IsAttackRecoveryEscapeState())
	{
		ExitAttackRecoveryForEscape(bKeepQueuedAttack);
		return;
	}

	if (IsDamageReactionRecoveryEscapeState())
	{
		ExitDamageReactionRecoveryForEscape();
	}
}

void ABAPlayerCharacter::ExitAttackRecoveryForEscape(const bool bKeepQueuedAttack)
{
	ClearRecoveryEscapeWindow();
	ClearAttackHitStop(true);
	GetWorldTimerManager().ClearTimer(ChargeAttackTimerHandle);

	bIsBeforeCharge = false;
	bIsCharging = false;
	bIsChargeInputCompleted = false;
	PausedMontage = nullptr;

	UAnimMontage* MontageToStop = ActiveAttackMontage;
	ActiveAttackMontage = nullptr;
	ActiveAttackPlaybackId = 0;

	if (!bKeepQueuedAttack)
	{
		NowComboTransitionTid = 0;
		NextComboTransitionTid = 0;
		NextAttackMontage = nullptr;
		NextAttackActionType = EActionType::None;
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			if (MontageToStop)
			{
				AnimInstance->Montage_Stop(RecoveryEscapeMontageBlendOut, MontageToStop);
			}
		}
	}

	if (PlayerWeaponVFX)
	{
		PlayerWeaponVFX->DeactivateTrailNiagara();
	}

	if (BAPlayerState == EBAPlayerState::Attacking)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}
}

void ABAPlayerCharacter::ExitDamageReactionRecoveryForEscape()
{
	ClearRecoveryEscapeWindow();
	GetWorldTimerManager().ClearTimer(DamageReactionTimerHandle);

	UAnimMontage* MontageToStop = ActiveDamageReactionMontage;
	ActiveDamageReactionPlaybackId = 0;
	ActiveDamageReactionMontage = nullptr;
	DamageReactionState = EPlayerDamageReactionState::None;

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			if (MontageToStop)
			{
				AnimInstance->Montage_Stop(RecoveryEscapeMontageBlendOut, MontageToStop);
			}
		}
	}

	if (BAPlayerState == EBAPlayerState::HitReacting)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}
}
