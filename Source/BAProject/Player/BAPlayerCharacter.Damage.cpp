#include "Player/BAPlayerCharacter.h"

#include "Animation/AnimMontage.h"
#include "Component/ActionComponent.h"
#include "Component/StatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

namespace
{
	UAnimMontage* FindMontageForDirection(
		const TMap<EActionDirection, TObjectPtr<UAnimMontage>>& Montages,
		const EActionDirection Direction)
	{
		if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(Direction))
		{
			if (Montage->Get())
			{
				return Montage->Get();
			}
		}

		// 8방향 몽타주가 모두 없을 수 있어 대각선은 인접 축 방향으로 보정한다.
		switch (Direction)
		{
		case EActionDirection::ForwardLeft:
			if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Forward))
			{
				if (Montage->Get())
				{
					return Montage->Get();
				}
			}
			if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Left))
			{
				if (Montage->Get())
				{
					return Montage->Get();
				}
			}
			break;
		case EActionDirection::ForwardRight:
			if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Forward))
			{
				if (Montage->Get())
				{
					return Montage->Get();
				}
			}
			if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Right))
			{
				if (Montage->Get())
				{
					return Montage->Get();
				}
			}
			break;
		case EActionDirection::BackwardLeft:
			if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Backward))
			{
				if (Montage->Get())
				{
					return Montage->Get();
				}
			}
			if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Left))
			{
				if (Montage->Get())
				{
					return Montage->Get();
				}
			}
			break;
		case EActionDirection::BackwardRight:
			if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Backward))
			{
				if (Montage->Get())
				{
					return Montage->Get();
				}
			}
			if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Right))
			{
				if (Montage->Get())
				{
					return Montage->Get();
				}
			}
			break;
		default:
			break;
		}

		if (const TObjectPtr<UAnimMontage>* Montage = Montages.Find(EActionDirection::Any))
		{
			return Montage->Get();
		}

		return nullptr;
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

	if (StatComponent)
	{
		StatComponent->ApplyDamage(FinalDamage);
		if (StatComponent->IsDead())
		{
			return;
		}
	}

	const EBADamageReactionType DamageReactionType = ResolveDamageReactionType(DamageEvent);
	const FVector DamageDirection = ResolveDamageDirection(*this, DamageEvent, DamageCauser);
	const EActionDirection HitDirection = ResolveHitDirection(*this, DamageDirection);

	// 데미지 반영 후 생존 상태에서만 가드/브레이크/피격 반응을 결정한다.
	const bool bGuarding = IsGuardingAgainstDamage(DamageDirection);
	const bool bGuardBreak = bGuarding && ShouldPlayGuardBreakReaction();

	CancelCurrentActionForDamageReaction();
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
	if (!ActionComponent || ActionComponent->GetGuardState() == EGuardState::None)
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
	if (!bUseLaunchKnockbackForDamageReaction)
	{
		return;
	}

	float KnockbackStrength = HitReactKnockbackStrength;
	if (bGuardBreak)
	{
		KnockbackStrength = GuardBreakKnockbackStrength;
	}
	else if (bGuarding)
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

void ABAPlayerCharacter::FinishDamageReaction(const int32 PlaybackId)
{
	if (PlaybackId != ActiveDamageReactionPlaybackId)
	{
		return;
	}

	ActiveDamageReactionPlaybackId = 0;
	ActiveDamageReactionMontage = nullptr;
	if (DamageReactionState == EPlayerDamageReactionState::KnockDown)
	{
		SetInvincible(false);
	}
	DamageReactionState = EPlayerDamageReactionState::None;
	if (BAPlayerState == EBAPlayerState::HitReacting || BAPlayerState == EBAPlayerState::KnockedDown)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}
}
