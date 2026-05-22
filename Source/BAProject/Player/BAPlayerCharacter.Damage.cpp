#include "Player/BAPlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Component/ActionComponent.h"
#include "Component/StatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

/*
 * Damage Reaction Policy Summary
 *
 * 피격 처리는 "판정 결과", "스탯 반영", "리액션 연출", "상태 복귀"를 분리해서 다룬다.
 * 전투 판정 쪽에서는 피해 대상이 가드 중인지, 퍼펙트 가드 윈도우가 열려 있는지만 전달하고
 * 플레이어 Damage 모듈은 그 결과를 바탕으로 실제 피해량과 리액션을 결정한다.
 *
 * 1. 판정 우선순위
 * - 퍼펙트 가드는 최우선인 가드 계열 판정이다. 성공하면 HP 피해 없이 가드를 유지하고 피격 리액션으로 내려가지 않는다.
 * - 퍼펙트 가드 윈도우가 열려 있으면 일반 가드 윈도우 전이라도 가드 액션 중인 한 퍼펙트 판정 후보가 된다.
 * - 일반 가드는 스태미너를 소비하고, 가드 피해 감소율을 적용한 HP 피해를 받는다.
 * - 가드 스태미너가 부족하면 가드 브레이크가 된다. 피해 감소는 일반 가드와 같지만 긴 브레이크 리액션이 패널티다.
 * - 가드 조건을 만족하지 않으면 일반 피격으로 처리한다.
 *
 * 2. 스탯 반영
 * - 최종 피해량을 계산한 뒤 StatComponent에 HP 피해를 적용한다.
 * - 가드와 퍼펙트 가드의 스태미너 소비는 가드 정책에서 관리하는 액션 비용을 따른다.
 * - 스태미너 소비가 발생하면 회복 딜레이가 다시 적용되어야 한다.
 *
 * 3. 리액션과 이동
 * - 일반 피격, 큰 피격, 넉다운, 가드 히트, 가드 브레이크는 서로 다른 리액션 상태로 구분한다.
 * - 피격 리액션에 들어가면 현재 액션과 이동 페이즈를 끊고, Sprint도 Run으로 내려 스태미너 소모가 계속되지 않게 한다.
 * - 가드 브레이크는 긴 무방비 리액션과 루트모션 밀림으로 처리한다.
 * - 퍼펙트 가드를 포함한 그 외 피격/가드 히트는 Launch 넉백으로 밀림을 통일한다.
 * - 일반 가드 히트는 짧은 리액션 후 입력이 유지되어 있으면 다시 가드 루프로 복귀한다.
 *
 * 4. 연속 피격과 종료
 * - 연속 피격 시 이전 리액션 종료 타이머가 새 리액션을 종료하지 못하도록 재생 식별자를 갱신한다.
 * - GuardHit는 블렌드아웃이 시작되면 즉시 가드 복귀를 시도해 idle 노출을 줄인다.
 * - GuardBreak는 리액션 동안 무방비지만, 종료 시점에 입력이 유지되어 있으면 가드 재시작을 시도한다.
 * - 사망, 입력 해제, 사다리 상태, 스태미너 부족처럼 복귀 조건을 만족하지 못하면 가드 상태를 정리한다.
 */
namespace
{
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

void ABAPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (StatComponent)
	{
		StatComponent->OnDead.AddDynamic(this, &ABAPlayerCharacter::OnDeath);
	}
}

void ABAPlayerCharacter::OnDamaged(
	const float FinalDamage,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::OnDamaged(FinalDamage, DamageEvent, EventInstigator, DamageCauser);

	const EBADamageReactionType DamageReactionType = ResolveDamageReactionType(DamageEvent);
	const FVector DamageDirection = ResolveDamageDirection(*this, DamageEvent, DamageCauser);
	const EActionDirection HitDirection = ResolveHitDirection(*this, DamageDirection);
	const FBADamageEvent* BADamageEvent = DamageEvent.IsOfType(FBADamageEvent::ClassID)
		? static_cast<const FBADamageEvent*>(&DamageEvent)
		: nullptr;

	const bool bGuarding = BADamageEvent ? BADamageEvent->bVictimGuarding : IsGuardingAgainstDamage(DamageDirection);
	const bool bPerfectGuard = bGuarding && (BADamageEvent
		? BADamageEvent->bVictimPerfectGuard
		: IsPerfectGuardWindowActive());
	if (bPerfectGuard)
	{
		HandlePerfectGuardSucceeded(ResolveDamageHitResult(DamageEvent), DamageCauser);
		KeepGuardActiveAfterGuardSuccess();
		// 퍼펙트 가드는 GuardHit 몽타주를 재생하지 않으므로 넉백 속도가 Free 회전 입력처럼 해석되지 않게 막는다.
		MovementRuntime.bSuppressVelocityFacingUntilMoveInput = true;
		if (!MovementRuntime.bHasMoveInput)
		{
			SnapInterpolatedMoveInputTo(FVector2D::ZeroVector);
		}
		ApplyDamageReactionKnockback(DamageReactionType, DamageDirection, HitDirection, true, false);
		return;
	}

	bool bGuardBreak = false;
	float AppliedDamage = FinalDamage;
	if (bGuarding)
	{
		bGuardBreak = !ConsumeGuardStaminaForDamage();
		ShowGuardJudgementDebugMessage(
			bGuardBreak ? TEXT("Guard Break") : TEXT("Guard"),
			bGuardBreak ? FColor::Red : FColor::Yellow);
		AppliedDamage = FinalDamage * GetGuardAbsorptionMultiplier();
	}

	if (StatComponent)
	{
		StatComponent->ApplyDamage(AppliedDamage);
		if (StatComponent->IsDead())
		{
			return;
		}
	}

	CancelCurrentActionForDamageReaction();
	if (bGuarding && !bGuardBreak)
	{
		KeepGuardActiveAfterGuardSuccess();
	}
	ApplyDamageReactionKnockback(DamageReactionType, DamageDirection, HitDirection, bGuarding, bGuardBreak);
	PlayDamageReactionAnimation(DamageReactionType, HitDirection, bGuarding, bGuardBreak);
}

void ABAPlayerCharacter::OnDeath()
{
	Super::OnDeath();

	GetWorldTimerManager().ClearTimer(DamageReactionTimerHandle);
	DamageReactionState = EPlayerDamageReactionState::None;
	ActiveDamageReactionMontage = nullptr;
	ActiveDamageReactionPlaybackId = 0;
	bGuardInputHeld = false;
	SetGuardWindowActive(false);
	SetBAPlayerState(EBAPlayerState::Dead);

	if (ActionComponent)
	{
		ActionComponent->CancelCurrentAction();
	}
}

bool ABAPlayerCharacter::IsDamageReacting() const
{
	return DamageReactionState != EPlayerDamageReactionState::None;
}

EPlayerDamageReactionState ABAPlayerCharacter::GetDamageReactionState() const
{
	return DamageReactionState;
}

bool ABAPlayerCharacter::CanAcceptActionInput() const
{
	return IsAlive() && !IsDamageReacting() && !IsOnLadder();
}

bool ABAPlayerCharacter::IsGuardingAgainstDamage(const FVector& DamageDirection) const
{
	if (!ActionComponent)
	{
		return false;
	}

	const EGuardState GuardState = ActionComponent->GetGuardState();
	const bool bGuardStateActive = GuardState == EGuardState::Guarding || GuardState == EGuardState::Blocking;
	const bool bPerfectGuardActionWindowActive = bPerfectGuardWindowActive
		&& ActionComponent->GetActiveActionType() == EActionType::Guard;
	// GuardHit 리액션 중에는 가드 액션 몽타주가 끊겨 있어도 입력이 유지되면 방어 판정을 이어간다.
	const bool bGuardHitMaintainsGuard = DamageReactionState == EPlayerDamageReactionState::GuardHit
		&& ShouldResumeGuardAfterGuardReaction();
	if (!bGuardStateActive && !bPerfectGuardActionWindowActive && !bGuardHitMaintainsGuard)
	{
		return false;
	}

	FVector SourceDirection = -DamageDirection;
	SourceDirection.Z = 0.f;
	if (!SourceDirection.Normalize())
	{
		return true;
	}

	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;
	if (!Forward.Normalize())
	{
		return true;
	}

	const float HalfAngleRadians = FMath::DegreesToRadians(FMath::Clamp(GuardDamageBlockAngle, 0.f, 360.f) * 0.5f);
	const float MinDot = FMath::Cos(HalfAngleRadians);
	return FVector::DotProduct(Forward, SourceDirection) >= MinDot;
}

bool ABAPlayerCharacter::ShouldPlayGuardBreakReaction() const
{
	return ActionComponent && ActionComponent->GetGuardState() == EGuardState::GuardBroken;
}

void ABAPlayerCharacter::CancelCurrentActionForDamageReaction()
{
	SetGuardWindowActive(false);

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
	if (bGuarding)
	{
		KnockbackStrength = GuardHitKnockbackStrength;
	}
	else if (DamageReactionType == EBADamageReactionType::KnockDown)
	{
		KnockbackStrength = KnockDownKnockbackStrength;
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

	FVector LaunchVelocity = KnockbackDirection * KnockbackStrength;
	LaunchVelocity.Z = DamageReactionKnockbackZ;
	LaunchCharacter(LaunchVelocity, true, false);
}

void ABAPlayerCharacter::PlayDamageReactionAnimation(
	const EBADamageReactionType DamageReactionType,
	const EActionDirection HitDirection,
	const bool bGuarding,
	const bool bGuardBreak)
{
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
		if (DamageReactionState == EPlayerDamageReactionState::GuardHit && ReactionDuration > 0.f)
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
	}

	if (ReactionDuration <= 0.f)
	{
		FinishDamageReaction(PlaybackId);
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

void ABAPlayerCharacter::FinishDamageReaction(const int32 PlaybackId)
{
	if (PlaybackId != ActiveDamageReactionPlaybackId)
	{
		return;
	}

	const EPlayerDamageReactionState FinishedDamageReactionState = DamageReactionState;
	ActiveDamageReactionPlaybackId = 0;
	ActiveDamageReactionMontage = nullptr;
	if (DamageReactionState == EPlayerDamageReactionState::KnockDown)
	{
		SetInvincible(false);
	}

	const bool bFinishedGuardReaction = FinishedDamageReactionState == EPlayerDamageReactionState::GuardHit
		|| FinishedDamageReactionState == EPlayerDamageReactionState::GuardBreak;
	const bool bShouldResumeGuard = bFinishedGuardReaction && ShouldResumeGuardAfterGuardReaction();
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
