#include "Player/BAPlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Component/ActionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "TimerManager.h"

/*
 * Damage Hit Reaction Policy Summary
 *
 * 이 파일은 피격 리액션의 재생과 종료를 담당한다. 피해 판정과 HP 반영은 Damage.cpp에서 끝내고,
 * 여기서는 리액션 상태, 몽타주, 넉백, 종료 시 복귀 흐름만 처리한다.
 *
 * 1. 리액션 상태
 * - ResolveDamageReactionState는 가드 브레이크, 가드 히트, KnockDown, LargeHit, HitReact 순서로 상태를 정한다.
 * - KnockDown은 에어본과 바닥에 눕는 리액션을 같은 규칙 묶음으로 다룬다.
 * - KnockDown에 들어가면 기립이 끝날 때까지 무적을 유지한다.
 *
 * 2. 몽타주 선택
 * - SelectDamageReactionMontage는 피격 타입과 방향으로 몽타주를 고른다.
 * - 방향별 몽타주가 없으면 대각선은 인접한 축 방향으로 보정하고, 마지막으로 Any를 찾는다.
 * - KnockDownReactMontages가 없으면 LargeHitReactMontages로 fallback한다.
 *
 * 3. 넉백과 런치
 * - ApplyDamageReactionKnockback은 DamageEvent의 LaunchHorizontalSpeed/LaunchVerticalSpeed를 우선 사용한다.
 * - 기본 HitReact 넉백은 HitReactKnockbackStrength 값을 그대로 사용한다.
 * - KnockDown은 Launch 값이 0이면 플레이어 기본 KnockDownKnockbackStrength/KnockDownLaunchVerticalSpeed를 사용한다.
 * - 일반 피격/가드는 수평 속도만 적용해 지상 리액션이 Falling으로 전환되지 않게 한다.
 * - GuardBreak는 루트모션 밀림을 사용하므로 Launch 넉백을 적용하지 않는다.
 *
 * 4. 종료
 * - 연속 피격은 PlaybackId로 이전 종료 타이머가 새 리액션을 끊지 못하게 막는다.
 * - GuardHit는 블렌드아웃 시작 시 바로 가드 복귀를 시도한다.
 * - KnockDown은 최소 누운 프레임까지 진행되고 지면에 닿으면 마지막 프레임을 고정하고 GetUp 모듈로 넘긴다.
 * - 지면 체크가 늦어지면 블렌드아웃 콜백이 안전망으로 같은 흐름을 호출한다.
 * - KnockDown은 바로 idle로 돌아가지 않고 GetUp 모듈의 대기/기립 흐름으로 넘긴다.
 */
namespace
{
	const FName GroundDamageKnockbackRootMotionName(TEXT("BA_GroundDamageKnockback"));

	UAnimMontage* FindConfiguredMontage(
		const TMap<EActionDirection, TObjectPtr<UAnimMontage>>& Montages,
		const EActionDirection Direction)
	{
		if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(Direction))
		{
			return Montage->Get();
		}

		return nullptr;
	}

	UAnimMontage* FindMontageForDirection(
		const TMap<EActionDirection, TObjectPtr<UAnimMontage>>& Montages,
		const EActionDirection Direction)
	{
		if (UAnimMontage* Montage = FindConfiguredMontage(Montages, Direction))
		{
			return Montage;
		}

		// 8방향 몽타주가 모두 없을 수 있어 대각선은 인접 축 방향으로 보정한다.
		switch (Direction)
		{
		case EActionDirection::ForwardLeft:
			if (UAnimMontage* Montage = FindConfiguredMontage(Montages, EActionDirection::Forward))
			{
				return Montage;
			}
			if (UAnimMontage* Montage = FindConfiguredMontage(Montages, EActionDirection::Left))
			{
				return Montage;
			}
			break;
		case EActionDirection::ForwardRight:
			if (UAnimMontage* Montage = FindConfiguredMontage(Montages, EActionDirection::Forward))
			{
				return Montage;
			}
			if (UAnimMontage* Montage = FindConfiguredMontage(Montages, EActionDirection::Right))
			{
				return Montage;
			}
			break;
		case EActionDirection::BackwardLeft:
			if (UAnimMontage* Montage = FindConfiguredMontage(Montages, EActionDirection::Backward))
			{
				return Montage;
			}
			if (UAnimMontage* Montage = FindConfiguredMontage(Montages, EActionDirection::Left))
			{
				return Montage;
			}
			break;
		case EActionDirection::BackwardRight:
			if (UAnimMontage* Montage = FindConfiguredMontage(Montages, EActionDirection::Backward))
			{
				return Montage;
			}
			if (UAnimMontage* Montage = FindConfiguredMontage(Montages, EActionDirection::Right))
			{
				return Montage;
			}
			break;
		default:
			break;
		}

		return FindConfiguredMontage(Montages, EActionDirection::Any);
	}

	FVector GetKnockbackDirectionFromHitDirection(const AActor& Actor, const EActionDirection HitDirection)
	{
		const FVector Forward = Actor.GetActorForwardVector();
		const FVector Right = Actor.GetActorRightVector();

		switch (HitDirection)
		{
		case EActionDirection::Forward:
			return -Forward;
		case EActionDirection::Backward:
			return Forward;
		case EActionDirection::Left:
			return Right;
		case EActionDirection::Right:
			return -Right;
		case EActionDirection::ForwardLeft:
			return (-Forward + Right).GetSafeNormal();
		case EActionDirection::ForwardRight:
			return (-Forward - Right).GetSafeNormal();
		case EActionDirection::BackwardLeft:
			return (Forward + Right).GetSafeNormal();
		case EActionDirection::BackwardRight:
			return (Forward - Right).GetSafeNormal();
		case EActionDirection::Any:
		default:
			return -Forward;
		}
	}
}

bool ABAPlayerCharacter::ShouldPlayGuardBreakReaction() const
{
	return ActionComponent && ActionComponent->GetGuardState() == EGuardState::GuardBroken;
}

void ABAPlayerCharacter::CancelCurrentActionForDamageReaction()
{
	SetGuardWindowActive(false);
	ClearAttackRuntimeState();

	// 전력질주 중 피격 시 전력질주가 끊겨야 함
	if (MovementRuntime.DesiredGait == EMovementState::Sprint)
	{
		SetMovementState(EMovementState::Run);
	}
	if (MovementRuntime.ActiveGait == EMovementState::Sprint)
	{
		SetActiveGaitAndSpeed(EMovementState::Run);
	}

	if (ActionComponent)
	{
		// 액션 종료 이벤트를 통해 ActionAnimationComponent가 현재 액션 몽타주를 정리한다.
		ActionComponent->CancelCurrentAction();
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	MovementRuntime.Phase = EPlayerMovementPhase::None;
	MovementRuntime.PhaseElapsedTime = 0.f;
	MovementRuntime.bWaitingForPhaseAnimation = false;
}

void ABAPlayerCharacter::ApplyDamageReactionKnockback(
	const EBADamageReactionType DamageReactionType,
	const FVector& DamageDirection,
	const EActionDirection HitDirection,
	const bool bGuarding,
	const bool bGuardBreak)
{
	if (bGuardBreak)
	{
		// GuardBreak는 긴 무방비 리액션의 루트모션으로 밀림을 표현한다.
		return;
	}

	// 넉백 강도는 플레이어가 보유한 피격 반응 타입별 값으로 결정한다.
	float KnockbackStrength = HitReactKnockbackStrength;
	bool bUseLaunchKnockback = false;
	if (bGuarding)
	{
		KnockbackStrength = GuardHitKnockbackStrength;
	}
	else if (DamageReactionType == EBADamageReactionType::KnockDown)
	{
		bUseLaunchKnockback = true;
		KnockbackStrength = LastDamageLaunchHorizontalSpeed > 0.f
			? LastDamageLaunchHorizontalSpeed
			: KnockDownKnockbackStrength;
	}
	else if (DamageReactionType == EBADamageReactionType::LargeHitReact)
	{
		KnockbackStrength = LargeHitReactKnockbackStrength;
	}

	if (KnockbackStrength <= 0.f)
	{
		return;
	}

	FVector KnockbackDirection = DamageDirection;
	KnockbackDirection.Z = 0.f;
	if (!KnockbackDirection.Normalize())
	{
		KnockbackDirection = GetKnockbackDirectionFromHitDirection(*this, HitDirection);
		KnockbackDirection.Z = 0.f;
		KnockbackDirection.Normalize();
	}

	if (KnockbackDirection.IsNearlyZero())
	{
		return;
	}

	if (!bUseLaunchKnockback)
	{
		ApplyGroundDamageReactionKnockback(KnockbackDirection, KnockbackStrength);
		return;
	}

	const float KnockbackZ = LastDamageLaunchVerticalSpeed > 0.f
		? LastDamageLaunchVerticalSpeed
		: KnockDownLaunchVerticalSpeed;
	FVector LaunchVelocity = KnockbackDirection * KnockbackStrength;
	LaunchVelocity.Z = KnockbackZ;
	LaunchCharacter(LaunchVelocity, true, true);
}

void ABAPlayerCharacter::ApplyGroundDamageReactionKnockback(
	const FVector& KnockbackDirection,
	const float KnockbackStrength)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || KnockbackStrength <= 0.f)
	{
		return;
	}

	FVector GroundKnockbackDirection = KnockbackDirection;
	GroundKnockbackDirection.Z = 0.f;
	if (!GroundKnockbackDirection.Normalize())
	{
		return;
	}

	MovementComponent->RemoveRootMotionSource(GroundDamageKnockbackRootMotionName);

	if (GroundDamageReactionKnockbackDuration <= 0.f)
	{
		FVector GroundKnockbackVelocity = GroundKnockbackDirection * KnockbackStrength;
		GroundKnockbackVelocity.Z = MovementComponent->Velocity.Z;
		MovementComponent->Velocity = GroundKnockbackVelocity;
		return;
	}

	TSharedPtr<FRootMotionSource_ConstantForce> KnockbackRootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	KnockbackRootMotion->InstanceName = GroundDamageKnockbackRootMotionName;
	KnockbackRootMotion->AccumulateMode = ERootMotionAccumulateMode::Additive;
	KnockbackRootMotion->Priority = 500;
	KnockbackRootMotion->Duration = GroundDamageReactionKnockbackDuration;
	KnockbackRootMotion->Force = GroundKnockbackDirection * KnockbackStrength;
	KnockbackRootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	KnockbackRootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	MovementComponent->ApplyRootMotionSource(KnockbackRootMotion);
}

void ABAPlayerCharacter::PlayDamageReactionAnimation(
	const EBADamageReactionType DamageReactionType,
	const EActionDirection HitDirection,
	const bool bGuarding,
	const bool bGuardBreak)
{
	ClearRecoveryEscapeWindow();
	ResetKnockDownRecovery();
	bFallTrackingActive = false;

	const int32 PlaybackId = NextDamageReactionPlaybackId++;
	ActiveDamageReactionPlaybackId = PlaybackId;
	DamageReactionState = ResolveDamageReactionState(DamageReactionType, bGuarding, bGuardBreak);
	SetBAPlayerState(DamageReactionState == EPlayerDamageReactionState::KnockDown
		? EBAPlayerState::KnockedDown
		: EBAPlayerState::HitReacting);

	if (DamageReactionState == EPlayerDamageReactionState::KnockDown)
	{
		SetInvincible(true);
	}
	PlayDamageReactionCameraShake(DamageReactionType, bGuarding, bGuardBreak);
	PlayDamageReactionForceFeedback(DamageReactionType, bGuarding, bGuardBreak);

	// 연속 피격 시 이전 종료 타이머가 새 반응 상태를 해제하지 못하게 식별자를 갱신한다.
	GetWorldTimerManager().ClearTimer(DamageReactionTimerHandle);

	ActiveDamageReactionMontage = SelectDamageReactionMontage(
		DamageReactionType,
		HitDirection,
		bGuarding,
		bGuardBreak);

	K2_OnDamageReaction(DamageReactionType, HitDirection, bGuarding, bGuardBreak);

	float ReactionDuration = DamageReactionFallbackDuration;
	if (ActiveDamageReactionMontage)
	{
		ReactionDuration = PlayAnimMontage(ActiveDamageReactionMontage);
		if (ReactionDuration > 0.f && bDeathFinalizationDeferred)
		{
			if (USkeletalMeshComponent* MeshComponent = GetMesh())
			{
				if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
				{
					FOnMontageBlendingOutStarted BlendingOutDelegate;
					BlendingOutDelegate.BindUObject(
						this,
						&ABAPlayerCharacter::HandleDeferredDeathReactionMontageBlendingOut,
						PlaybackId);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, ActiveDamageReactionMontage);
				}
			}
		}
		else if (DamageReactionState == EPlayerDamageReactionState::GuardHit && ReactionDuration > 0.f)
		{
			if (USkeletalMeshComponent* MeshComponent = GetMesh())
			{
				if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
				{
					// GuardHit가 BlendOut에 들어가면 슬롯 가중치가 빠지며 idle이 보일 수 있으므로,
					// 완전 종료를 기다리지 않고 즉시 가드 루프로 복귀한다.
					FOnMontageBlendingOutStarted BlendingOutDelegate;
					BlendingOutDelegate.BindUObject(
						this,
						&ABAPlayerCharacter::HandleGuardHitReactionMontageBlendingOut,
						PlaybackId);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, ActiveDamageReactionMontage);
				}
			}
		}
		else if (DamageReactionState == EPlayerDamageReactionState::KnockDown && ReactionDuration > 0.f)
		{
			if (USkeletalMeshComponent* MeshComponent = GetMesh())
			{
				if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
				{
					FOnMontageBlendingOutStarted BlendingOutDelegate;
					BlendingOutDelegate.BindUObject(
						this,
						&ABAPlayerCharacter::HandleKnockDownReactionMontageBlendingOut,
						PlaybackId);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, ActiveDamageReactionMontage);
				}
			}
		}
	}

	if (ReactionDuration <= 0.f)
	{
		FinishDamageReaction(PlaybackId);
		return;
	}

	if (DamageReactionState == EPlayerDamageReactionState::KnockDown)
	{
		ScheduleKnockDownRecoveryStart(PlaybackId, ReactionDuration);
		return;
	}

	GetWorldTimerManager().SetTimer(
		DamageReactionTimerHandle,
		FTimerDelegate::CreateUObject(this, &ABAPlayerCharacter::FinishDamageReaction, PlaybackId),
		ReactionDuration,
		false);
}

EPlayerDamageReactionState ABAPlayerCharacter::ResolveDamageReactionState(
	const EBADamageReactionType DamageReactionType,
	const bool bGuarding,
	const bool bGuardBreak) const
{
	if (bGuardBreak)
	{
		return EPlayerDamageReactionState::GuardBreak;
	}

	if (bGuarding)
	{
		return EPlayerDamageReactionState::GuardHit;
	}

	if (DamageReactionType == EBADamageReactionType::KnockDown)
	{
		return EPlayerDamageReactionState::KnockDown;
	}

	if (DamageReactionType == EBADamageReactionType::LargeHitReact)
	{
		return EPlayerDamageReactionState::LargeHitReact;
	}

	return EPlayerDamageReactionState::HitReact;
}

UAnimMontage* ABAPlayerCharacter::SelectDamageReactionMontage(
	const EBADamageReactionType DamageReactionType,
	const EActionDirection HitDirection,
	const bool bGuarding,
	const bool bGuardBreak) const
{
	if (bGuardBreak)
	{
		return FindMontageForDirection(GuardBreakReactMontages, HitDirection);
	}

	if (bGuarding)
	{
		return FindMontageForDirection(GuardHitReactMontages, HitDirection);
	}

	if (DamageReactionType == EBADamageReactionType::KnockDown)
	{
		if (UAnimMontage* KnockDownMontage = FindMontageForDirection(KnockDownReactMontages, HitDirection))
		{
			return KnockDownMontage;
		}
		return FindMontageForDirection(LargeHitReactMontages, HitDirection);
	}

	if (DamageReactionType == EBADamageReactionType::LargeHitReact)
	{
		return FindMontageForDirection(LargeHitReactMontages, HitDirection);
	}

	return FindMontageForDirection(HitReactMontages, HitDirection);
}

void ABAPlayerCharacter::HandleGuardHitReactionMontageBlendingOut(
	UAnimMontage* Montage,
	const bool /*bInterrupted*/,
	const int32 PlaybackId)
{
	if (PlaybackId != ActiveDamageReactionPlaybackId || Montage != ActiveDamageReactionMontage)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DamageReactionTimerHandle);
	FinishDamageReaction(PlaybackId);
}

void ABAPlayerCharacter::HandleDeferredDeathReactionMontageBlendingOut(
	UAnimMontage* Montage,
	const bool /*bInterrupted*/,
	const int32 PlaybackId)
{
	if (PlaybackId != ActiveDamageReactionPlaybackId || Montage != ActiveDamageReactionMontage)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DamageReactionTimerHandle);
	FinishDeferredDeath(LastDamageHitDirection);
}

void ABAPlayerCharacter::HandleKnockDownReactionMontageBlendingOut(
	UAnimMontage* Montage,
	const bool /*bInterrupted*/,
	const int32 PlaybackId)
{
	if (PlaybackId != ActiveDamageReactionPlaybackId || Montage != ActiveDamageReactionMontage)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DamageReactionTimerHandle);
	GetWorldTimerManager().ClearTimer(KnockDownRecoveryStartTimerHandle);
	FinishDamageReaction(PlaybackId);
}

void ABAPlayerCharacter::FinishDamageReaction(const int32 PlaybackId)
{
	if (PlaybackId != ActiveDamageReactionPlaybackId)
	{
		return;
	}

	ClearRecoveryEscapeWindow();
	const EPlayerDamageReactionState FinishedDamageReactionState = DamageReactionState;
	if (bDeathFinalizationDeferred)
	{
		FinishDeferredDeath(LastDamageHitDirection);
		return;
	}

	if (FinishedDamageReactionState == EPlayerDamageReactionState::KnockDown)
	{
		ActiveDamageReactionPlaybackId = 0;
		BeginKnockDownRecoveryWait();
		return;
	}

	ActiveDamageReactionPlaybackId = 0;
	ActiveDamageReactionMontage = nullptr;

	const bool bFinishedGuardHitReaction = FinishedDamageReactionState == EPlayerDamageReactionState::GuardHit;
	const bool bFinishedGuardBreakReaction = FinishedDamageReactionState == EPlayerDamageReactionState::GuardBreak;
	const bool bFinishedGuardReaction = bFinishedGuardHitReaction || bFinishedGuardBreakReaction;
	const bool bShouldResumeGuard = bFinishedGuardHitReaction
		? ShouldResumeGuardAfterGuardReaction()
		: bFinishedGuardBreakReaction && bGuardInputHeld && IsAlive() && !IsOnLadder();
	if (bFinishedGuardReaction && ActionComponent)
	{
		// GuardBreak 리액션 중에는 무방비였으므로 종료 시 상태를 비운 뒤, 입력이 유지되어 있으면 아래에서 재시작한다.
		ActionComponent->SetGuardState(EGuardState::None);
		SetCombatMode(EPlayerCombatMode::None);
	}
	DamageReactionState = EPlayerDamageReactionState::None;
	if (BAPlayerState == EBAPlayerState::HitReacting || BAPlayerState == EBAPlayerState::KnockedDown)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}

	if (bShouldResumeGuard)
	{
		if (!ResumeGuardAfterGuardReaction())
		{
			if (ActionComponent)
			{
				ActionComponent->SetGuardState(EGuardState::None);
			}
			SetCombatMode(EPlayerCombatMode::None);
		}
	}
}
