#include "Player/BAPlayerCharacter.h"

#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Component/ActionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

/*
 * Damage GetUp Policy Summary
 *
 * 이 파일은 KnockDown/Airborne 리액션이 끝난 뒤 기립까지의 상태를 관리한다.
 * 피격 리액션 재생은 Damage.Hit.cpp에서 끝내고, 여기서는 마지막 자세 고정, 무적 유지, 기립 복귀만 처리한다.
 *
 * 1. 대기 진입
 * - KnockDown 리액션은 몽타주가 최소 누운 프레임까지 진행되고 지면에 닿으면 회복 대기로 넘어간다.
 * - 회복 대기에 들어가면 ActiveDamageReactionMontage를 마지막 프레임에 멈춘다.
 * - 이 동안 DamageReactionState는 KnockDown으로 유지되어 이동과 액션 입력을 막는다.
 * - 무적은 계속 유지한다. 다운된 상태에서 추가 피격을 받지 않게 하기 위한 규칙이다.
 *
 * 2. 입력이 없을 때
 * - KnockDownGetUpEscapeInputStartDelay 전까지는 입력 탈출을 받지 않는다. 기본값은 0.1초다.
 * - KnockDownGetUpEscapeInputStartDelay부터 KnockDownGetUpEscapeInputEndDelay까지 입력 탈출 창을 연다.
 * - 기본 입력 탈출 창은 0.1초부터 2.0초까지다.
 * - 입력 탈출 창이 끝나면 KnockDownGetUpNoInputDelayAfterEscapeWindow 동안 더 누워 있다.
 * - 기본값 기준 아무 입력이 없으면 전체 2.5초 뒤에 기립한다.
 * - 전체 대기 시간이 끝나면 KnockDownGetUpMontage를 재생하고, 끝나면 idle로 돌아간다.
 *
 * 3. 이동 입력이 있을 때
 * - 입력 탈출 창 안에서 이동 입력이 들어오면 기립 몽타주를 지정 비율만 재생한 뒤 이동으로 탈출한다.
 * - 기본값은 50%다.
 * - 구르기 입력도 기립 몽타주를 재생하지 않고 바로 구르기로 탈출한다.
 * - 구르기 버튼을 창이 열리기 전에 누르고 유지한 경우, 창이 열리는 순간 입력을 소비한다.
 *
 * 4. 기립 종료
 * - 기립 시작 시 멈춰 있던 피격 리액션 몽타주를 블렌드 없이 정리하고 Mesh pause를 해제한다.
 * - 기립 몽타주는 blend-in 0으로 시작한다. 마지막 누운 자세와 idle 사이가 노출되지 않게 하기 위한 규칙이다.
 * - 기립이 끝나면 DamageReactionState와 BAPlayerState를 비우고 무적을 해제한다.
 * - 이동 입력이 유지되어 있으면 Movement Phase를 Start 또는 Loop로 다시 시작한다.
 */
namespace
{
	constexpr float KnockDownRecoveryGroundRetryInterval = 0.05f;
}

void ABAPlayerCharacter::ResetKnockDownRecovery()
{
	GetWorldTimerManager().ClearTimer(KnockDownRecoveryStartTimerHandle);
	GetWorldTimerManager().ClearTimer(KnockDownGetUpTimerHandle);
	bKnockDownWaitingForGetUp = false;
	bKnockDownGetUpInProgress = false;
	bKnockDownGetUpEscapeWindowOpen = false;
	bKnockDownGetUpQueuedByMoveInput = false;
	bKnockDownGetUpQueuedDodgeInput = false;
	bKnockDownGetUpDodgeInputHeld = false;
	bKnockDownGetUpMoveInputShortcut = false;
	KnockDownGetUpQueuedDodgeDirection = EActionDirection::Any;
	KnockDownGetUpHeldDodgeDirection = EActionDirection::Any;
	ActiveKnockDownGetUpMontage = nullptr;
}

void ABAPlayerCharacter::ScheduleKnockDownRecoveryStart(const int32 PlaybackId, const float ReactionDuration)
{
	if (PlaybackId != ActiveDamageReactionPlaybackId || DamageReactionState != EPlayerDamageReactionState::KnockDown)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(KnockDownRecoveryStartTimerHandle);

	const float ClampedReactionDuration = FMath::Max(0.f, ReactionDuration);
	const float MinMontageTime = FMath::Max(0.f, KnockDownRecoveryMinMontageTime);
	const float RecoveryCheckDelay = ClampedReactionDuration > 0.f
		? FMath::Min(MinMontageTime, ClampedReactionDuration)
		: MinMontageTime;
	if (RecoveryCheckDelay <= 0.f)
	{
		TryBeginKnockDownRecoveryWait(PlaybackId);
		return;
	}

	GetWorldTimerManager().SetTimer(
		KnockDownRecoveryStartTimerHandle,
		FTimerDelegate::CreateUObject(this, &ABAPlayerCharacter::TryBeginKnockDownRecoveryWait, PlaybackId),
		RecoveryCheckDelay,
		false);
}

void ABAPlayerCharacter::TryBeginKnockDownRecoveryWait(const int32 PlaybackId)
{
	if (PlaybackId != ActiveDamageReactionPlaybackId || DamageReactionState != EPlayerDamageReactionState::KnockDown)
	{
		return;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent && MovementComponent->IsFalling())
	{
		GetWorldTimerManager().SetTimer(
			KnockDownRecoveryStartTimerHandle,
			FTimerDelegate::CreateUObject(this, &ABAPlayerCharacter::TryBeginKnockDownRecoveryWait, PlaybackId),
			KnockDownRecoveryGroundRetryInterval,
			false);
		return;
	}

	GetWorldTimerManager().ClearTimer(KnockDownRecoveryStartTimerHandle);
	GetWorldTimerManager().ClearTimer(DamageReactionTimerHandle);
	FinishDamageReaction(PlaybackId);
}

void ABAPlayerCharacter::BeginKnockDownRecoveryWait()
{
	const bool bWasDodgeInputHeld = bKnockDownGetUpDodgeInputHeld;
	const EActionDirection HeldDodgeDirection = KnockDownGetUpHeldDodgeDirection;

	GetWorldTimerManager().ClearTimer(KnockDownRecoveryStartTimerHandle);
	ActiveDamageReactionPlaybackId = 0;
	FreezeMontageAtFinalFrame(ActiveDamageReactionMontage);
	bKnockDownWaitingForGetUp = true;
	bKnockDownGetUpInProgress = false;
	bKnockDownGetUpEscapeWindowOpen = false;
	bKnockDownGetUpQueuedByMoveInput = false;
	bKnockDownGetUpQueuedDodgeInput = false;
	bKnockDownGetUpDodgeInputHeld = bWasDodgeInputHeld;
	bKnockDownGetUpMoveInputShortcut = false;
	KnockDownGetUpQueuedDodgeDirection = EActionDirection::Any;
	KnockDownGetUpHeldDodgeDirection = bWasDodgeInputHeld ? HeldDodgeDirection : EActionDirection::Any;

	MovementRuntime.Phase = EPlayerMovementPhase::None;
	MovementRuntime.PhaseElapsedTime = 0.f;
	MovementRuntime.bWaitingForPhaseAnimation = false;

	const float EscapeStartDelay = FMath::Max(0.f, KnockDownGetUpEscapeInputStartDelay);
	if (EscapeStartDelay <= 0.f)
	{
		OpenKnockDownGetUpEscapeWindow();
		return;
	}

	GetWorldTimerManager().SetTimer(
		KnockDownGetUpTimerHandle,
		this,
		&ABAPlayerCharacter::OpenKnockDownGetUpEscapeWindow,
		EscapeStartDelay,
		false);
}

bool ABAPlayerCharacter::RequestKnockDownGetUpEscape()
{
	return TryStartKnockDownGetUpEscape(false, EActionDirection::Any);
}

bool ABAPlayerCharacter::RequestKnockDownGetUpDodgeEscape(const EActionDirection DodgeDirection)
{
	return TryStartKnockDownGetUpEscape(true, DodgeDirection);
}

void ABAPlayerCharacter::SetKnockDownGetUpDodgeInputHeld(
	const bool bHeld,
	const EActionDirection DodgeDirection)
{
	if (bKnockDownGetUpInProgress || DamageReactionState != EPlayerDamageReactionState::KnockDown)
	{
		return;
	}

	bKnockDownGetUpDodgeInputHeld = bHeld;
	KnockDownGetUpHeldDodgeDirection = bHeld ? DodgeDirection : EActionDirection::Any;
	if (bHeld && bKnockDownGetUpEscapeWindowOpen)
	{
		RequestKnockDownGetUpDodgeEscape(DodgeDirection);
	}
}

bool ABAPlayerCharacter::TryStartKnockDownGetUpEscape(
	const bool bQueueDodge,
	const EActionDirection DodgeDirection)
{
	if (!bKnockDownWaitingForGetUp
		|| bKnockDownGetUpInProgress
		|| !bKnockDownGetUpEscapeWindowOpen
		|| !IsAlive())
	{
		return false;
	}

	bKnockDownGetUpQueuedByMoveInput = true;
	if (bQueueDodge)
	{
		bKnockDownGetUpQueuedDodgeInput = true;
		KnockDownGetUpQueuedDodgeDirection = DodgeDirection;
		EscapeKnockDownGetUpImmediately();
		return true;
	}

	StartKnockDownGetUp();
	return true;
}

void ABAPlayerCharacter::OpenKnockDownGetUpEscapeWindow()
{
	if (!bKnockDownWaitingForGetUp || bKnockDownGetUpInProgress)
	{
		return;
	}

	bKnockDownGetUpEscapeWindowOpen = true;

	if (bKnockDownGetUpDodgeInputHeld)
	{
		RequestKnockDownGetUpDodgeEscape(KnockDownGetUpHeldDodgeDirection);
		return;
	}

	if (MovementRuntime.bHasMoveInput)
	{
		RequestKnockDownGetUpEscape();
		return;
	}

	GetWorldTimerManager().ClearTimer(KnockDownGetUpTimerHandle);
	const float EscapeStartDelay = FMath::Max(0.f, KnockDownGetUpEscapeInputStartDelay);
	const float EscapeEndDelay = FMath::Max(EscapeStartDelay, KnockDownGetUpEscapeInputEndDelay);
	const float EscapeWindowDuration = EscapeEndDelay - EscapeStartDelay;
	if (EscapeWindowDuration <= 0.f)
	{
		CloseKnockDownGetUpEscapeWindow();
		return;
	}

	GetWorldTimerManager().SetTimer(
		KnockDownGetUpTimerHandle,
		this,
		&ABAPlayerCharacter::CloseKnockDownGetUpEscapeWindow,
		EscapeWindowDuration,
		false);
}

void ABAPlayerCharacter::CloseKnockDownGetUpEscapeWindow()
{
	if (!bKnockDownWaitingForGetUp || bKnockDownGetUpInProgress)
	{
		return;
	}

	bKnockDownGetUpEscapeWindowOpen = false;
	bKnockDownGetUpQueuedByMoveInput = false;
	bKnockDownGetUpQueuedDodgeInput = false;
	bKnockDownGetUpDodgeInputHeld = false;
	KnockDownGetUpQueuedDodgeDirection = EActionDirection::Any;
	KnockDownGetUpHeldDodgeDirection = EActionDirection::Any;

	GetWorldTimerManager().ClearTimer(KnockDownGetUpTimerHandle);
	const float LockedProneDelay = FMath::Max(0.f, KnockDownGetUpNoInputDelayAfterEscapeWindow);
	if (LockedProneDelay <= 0.f)
	{
		StartKnockDownGetUp();
		return;
	}

	GetWorldTimerManager().SetTimer(
		KnockDownGetUpTimerHandle,
		this,
		&ABAPlayerCharacter::StartKnockDownGetUp,
		LockedProneDelay,
		false);
}

void ABAPlayerCharacter::EscapeKnockDownGetUpImmediately()
{
	if (!bKnockDownWaitingForGetUp || !IsAlive())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(KnockDownGetUpTimerHandle);
	bKnockDownWaitingForGetUp = false;
	bKnockDownGetUpInProgress = true;
	bKnockDownGetUpMoveInputShortcut = true;
	bKnockDownGetUpEscapeWindowOpen = false;
	ActiveKnockDownGetUpMontage = nullptr;

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			if (ActiveDamageReactionMontage)
			{
				AnimInstance->Montage_Stop(0.f, ActiveDamageReactionMontage);
				ActiveDamageReactionMontage = nullptr;
			}
		}

		MeshComponent->bPauseAnims = false;
	}

	FinishKnockDownGetUp();
}

void ABAPlayerCharacter::RefreshKnockDownGetUpForMoveInput()
{
	if (!bKnockDownWaitingForGetUp || !MovementRuntime.bHasMoveInput)
	{
		return;
	}

	RequestKnockDownGetUpEscape();
}

void ABAPlayerCharacter::StartKnockDownGetUp()
{
	if (!bKnockDownWaitingForGetUp || !IsAlive())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(KnockDownGetUpTimerHandle);
	bKnockDownWaitingForGetUp = false;
	bKnockDownGetUpInProgress = true;
	bKnockDownGetUpMoveInputShortcut = bKnockDownGetUpQueuedByMoveInput;
	bKnockDownGetUpEscapeWindowOpen = false;
	bKnockDownGetUpQueuedByMoveInput = false;
	ActiveKnockDownGetUpMontage = KnockDownGetUpMontage;

	float GetUpDuration = 0.f;
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* AnimInstance = nullptr;
	if (MeshComponent)
	{
		AnimInstance = MeshComponent->GetAnimInstance();
		if (AnimInstance)
		{
			if (ActiveDamageReactionMontage)
			{
				AnimInstance->Montage_Stop(0.f, ActiveDamageReactionMontage);
				ActiveDamageReactionMontage = nullptr;
			}
		}
	}

	if (ActiveKnockDownGetUpMontage && AnimInstance)
	{
		const FAlphaBlendArgs InstantBlendIn(0.f);
		GetUpDuration = AnimInstance->Montage_PlayWithBlendIn(
			ActiveKnockDownGetUpMontage,
			InstantBlendIn,
			1.f,
			EMontagePlayReturnType::Duration,
			0.f,
			true);
		if (GetUpDuration > 0.f)
		{
			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(this, &ABAPlayerCharacter::HandleKnockDownGetUpMontageEnded);
			AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveKnockDownGetUpMontage);
		}
	}

	if (MeshComponent)
	{
		MeshComponent->bPauseAnims = false;
	}

	if (GetUpDuration <= 0.f)
	{
		FinishKnockDownGetUp();
		return;
	}

	const float FinishFraction = bKnockDownGetUpMoveInputShortcut
		? FMath::Clamp(KnockDownGetUpMoveInputMontageFraction, 0.f, 1.f)
		: 1.f;
	const float FinishDelay = GetUpDuration * FinishFraction;
	if (FinishDelay <= 0.f)
	{
		FinishKnockDownGetUp();
		return;
	}

	GetWorldTimerManager().SetTimer(
		KnockDownGetUpTimerHandle,
		this,
		&ABAPlayerCharacter::FinishKnockDownGetUp,
		FinishDelay,
		false);
}

void ABAPlayerCharacter::FinishKnockDownGetUp()
{
	if (!bKnockDownGetUpInProgress && !bKnockDownWaitingForGetUp)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(KnockDownGetUpTimerHandle);
	GetWorldTimerManager().ClearTimer(KnockDownRecoveryStartTimerHandle);

	UAnimMontage* MontageToStop = bKnockDownGetUpMoveInputShortcut
		? ActiveKnockDownGetUpMontage.Get()
		: nullptr;
	const bool bImmediateInputEscape = bKnockDownGetUpMoveInputShortcut;
	const bool bShouldStartQueuedDodge = bKnockDownGetUpMoveInputShortcut && bKnockDownGetUpQueuedDodgeInput;
	const EActionDirection QueuedDodgeDirection = KnockDownGetUpQueuedDodgeDirection;

	bKnockDownWaitingForGetUp = false;
	bKnockDownGetUpInProgress = false;
	bKnockDownGetUpEscapeWindowOpen = false;
	bKnockDownGetUpQueuedByMoveInput = false;
	bKnockDownGetUpQueuedDodgeInput = false;
	bKnockDownGetUpDodgeInputHeld = false;
	bKnockDownGetUpMoveInputShortcut = false;
	KnockDownGetUpQueuedDodgeDirection = EActionDirection::Any;
	KnockDownGetUpHeldDodgeDirection = EActionDirection::Any;
	ActiveKnockDownGetUpMontage = nullptr;
	ActiveDamageReactionPlaybackId = 0;
	ActiveDamageReactionMontage = nullptr;
	DamageReactionState = EPlayerDamageReactionState::None;
	SetInvincible(false);

	if (BAPlayerState != EBAPlayerState::Dead)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}

	if (MontageToStop)
	{
		if (USkeletalMeshComponent* MeshComponent = GetMesh())
		{
			if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(KnockDownGetUpStopBlendOut, MontageToStop);
			}
		}
	}

	if (bShouldStartQueuedDodge && ActionComponent)
	{
		if (ActionComponent->TryStartAction(EActionCommand::Dodge, QueuedDodgeDirection))
		{
			return;
		}
	}

	if (MovementRuntime.bHasMoveInput)
	{
		if (bImmediateInputEscape)
		{
			SnapInterpolatedMoveInputTo(MovementRuntime.MoveInputVector);
			BeginMovementPhase(EPlayerMovementPhase::Loop);
			return;
		}

		BeginMovementPhase(IsPhaseEnabledForGait(EPlayerMovementPhase::Start, MovementRuntime.ActiveGait)
			? EPlayerMovementPhase::Start
			: EPlayerMovementPhase::Loop);
	}
}

void ABAPlayerCharacter::HandleKnockDownGetUpMontageEnded(UAnimMontage* Montage, const bool /*bInterrupted*/)
{
	if (!bKnockDownGetUpInProgress || Montage != ActiveKnockDownGetUpMontage)
	{
		return;
	}

	FinishKnockDownGetUp();
}
