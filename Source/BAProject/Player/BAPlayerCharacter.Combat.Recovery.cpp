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

	TryConsumeQueuedRecoveryEscapeAction(Settings);
}

void ABAPlayerCharacter::CloseRecoveryEscapeWindow(const FBAPlayerRecoveryEscapeWindowSettings& Settings)
{
	RecoveryEscapeWindowCount = FMath::Max(0, RecoveryEscapeWindowCount - 1);
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
	return IsAlive()
		&& (IsAttackRecoveryEscapeState()
			|| IsDamageReactionRecoveryEscapeState()
			|| IsDodgeRecoveryEscapeState());
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
		// 공격 간 전환은 AN_PlayerNextComboCheck Notify에서만 실행한다.
		return false;
	}

	if (Command == EActionCommand::Dodge)
	{
		if (IsDodgeRecoveryEscapeState() && !CanChainDodgeRecoveryEscape())
		{
			ClearQueuedRecoveryEscapeAction(Command);
			return false;
		}

		if (!CanUseRecoveryEscapeDodge())
		{
			QueueRecoveryEscapeAction(Command, Direction);
			return false;
		}

		const bool bStartFromDodgeChainSection = IsDodgeRecoveryEscapeState();
		ExitCurrentRecoveryForEscape(false);
		if (!ActionComponent)
		{
			bUseChainStartForNextDodgeAction = false;
			return false;
		}

		bUseChainStartForNextDodgeAction = bStartFromDodgeChainSection;
		const bool bStarted = Direction == EActionDirection::Any
			? ActionComponent->TryStartActionForRecoveryEscape(EActionCommand::Dodge, Direction)
			: ActionComponent->TryStartActionExcludingTypeForRecoveryEscape(
				EActionCommand::Dodge,
				Direction,
				EActionType::Backstep);
		if (!bStarted)
		{
			bUseChainStartForNextDodgeAction = false;
		}
		return bStarted;
	}

	if (Command == EActionCommand::Guard)
	{
		if (!CanUseRecoveryEscapeGuard())
		{
			QueueRecoveryEscapeAction(Command, Direction);
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

	const bool bDodgeRecoveryState = IsDodgeRecoveryEscapeState();
	if (bDodgeRecoveryState)
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
	const EPlayerMovementPhase MovementPhaseAfterEscape =
		IsPhaseEnabledForGait(EPlayerMovementPhase::Start, MovementRuntime.ActiveGait)
			? EPlayerMovementPhase::Start
			: EPlayerMovementPhase::Loop;
	BeginMovementPhase(MovementPhaseAfterEscape);
	return true;
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

bool ABAPlayerCharacter::IsDodgeRecoveryEscapeState() const
{
	if (BAPlayerState != EBAPlayerState::DodgeRolling || !ActionComponent)
	{
		return false;
	}

	return ActionComponent->GetActiveActionCommand() == EActionCommand::Dodge
		|| IsDodgeAction(ActionComponent->GetActiveActionType());
}

bool ABAPlayerCharacter::CanChainDodgeRecoveryEscape() const
{
	constexpr int32 MaxConsecutiveDodgeActions = 2;
	return ConsecutiveDodgeActionCount < MaxConsecutiveDodgeActions;
}

bool ABAPlayerCharacter::CanUseRecoveryEscapeDodge() const
{
	return IsRecoveryEscapeRequiredForCurrentState()
		&& IsRecoveryEscapeWindowOpen()
		&& RecoveryEscapeDodgeWindowCount > 0
		&& (!IsDodgeRecoveryEscapeState() || CanChainDodgeRecoveryEscape());
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

bool ABAPlayerCharacter::QueueRecoveryEscapeAction(
	const EActionCommand Command,
	const EActionDirection Direction)
{
	if (bConsumingQueuedRecoveryEscapeAction || !IsRecoveryEscapeRequiredForCurrentState())
	{
		return false;
	}

	if (Command != EActionCommand::Dodge && Command != EActionCommand::Guard)
	{
		return false;
	}

	// 단일 슬롯 큐처럼 새 입력이 기존 예약을 밀어내고, 윈도우가 열릴 때 가장 최근 입력만 실행한다.
	QueuedRecoveryEscapeCommand = Command;
	QueuedRecoveryEscapeDirection = Direction;
	return true;
}

bool ABAPlayerCharacter::TryConsumeQueuedRecoveryEscapeAction(const FBAPlayerRecoveryEscapeWindowSettings& Settings)
{
	if (QueuedRecoveryEscapeCommand == EActionCommand::None
		|| !CanQueuedRecoveryEscapeActionUseWindow(Settings))
	{
		return false;
	}

	const EActionCommand CommandToStart = QueuedRecoveryEscapeCommand;
	const EActionDirection DirectionToStart = QueuedRecoveryEscapeDirection;
	ClearQueuedRecoveryEscapeAction();

	TGuardValue<bool> ConsumingGuard(bConsumingQueuedRecoveryEscapeAction, true);
	return TryStartRecoveryEscapeAction(CommandToStart, DirectionToStart);
}

bool ABAPlayerCharacter::CanQueuedRecoveryEscapeActionUseWindow(
	const FBAPlayerRecoveryEscapeWindowSettings& Settings) const
{
	if (QueuedRecoveryEscapeCommand == EActionCommand::Dodge)
	{
		return Settings.bAllowDodge && CanUseRecoveryEscapeDodge();
	}

	if (QueuedRecoveryEscapeCommand == EActionCommand::Guard)
	{
		return Settings.bAllowGuard && CanUseRecoveryEscapeGuard();
	}

	return false;
}

void ABAPlayerCharacter::ClearRecoveryEscapeWindow()
{
	RecoveryEscapeWindowCount = 0;
	RecoveryEscapeDodgeWindowCount = 0;
	RecoveryEscapeGuardWindowCount = 0;
	RecoveryEscapeMoveWindowCount = 0;
	ClearQueuedRecoveryEscapeAction();
}

void ABAPlayerCharacter::ClearQueuedRecoveryEscapeAction()
{
	QueuedRecoveryEscapeCommand = EActionCommand::None;
	QueuedRecoveryEscapeDirection = EActionDirection::Any;
}

void ABAPlayerCharacter::ClearQueuedRecoveryEscapeAction(const EActionCommand Command)
{
	if (QueuedRecoveryEscapeCommand == Command)
	{
		ClearQueuedRecoveryEscapeAction();
	}
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
		return;
	}

	if (IsDodgeRecoveryEscapeState())
	{
		ExitDodgeRecoveryForEscape();
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

void ABAPlayerCharacter::ExitDodgeRecoveryForEscape()
{
	ClearRecoveryEscapeWindow();

	if (ActionComponent)
	{
		TGuardValue<bool> CompletingGuard(bCompletingDodgeForRecoveryEscape, true);
		ActionComponent->CancelCurrentAction();
	}

	if (BAPlayerState == EBAPlayerState::DodgeRolling)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}
}

void ABAPlayerCharacter::ResetConsecutiveDodgeActions()
{
	ConsecutiveDodgeActionCount = 0;
	bUseChainStartForNextDodgeAction = false;
	ClearQueuedRecoveryEscapeAction(EActionCommand::Dodge);
}
