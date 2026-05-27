#include "Player/BAPlayerCharacter.h"

#include "Component/ActionComponent.h"
#include "Component/StatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

/*
 * Falling Policy Summary
 *
 * 낙하는 CharacterMovement의 Falling/Landed 이벤트를 기준으로 추적한다.
 * KnockDown/Airborne 같은 피격 런치는 Damage 모듈이 처리하므로 여기서 낙하 데미지로 다시 해석하지 않는다.
 *
 * 1. 낙하 시작
 * - Falling에서 현재 Z 위치를 저장한다.
 * - 사다리, 피격 리액션, 사망, 기립 중에는 낙하 추적을 시작하지 않는다.
 *
 * 2. 착지
 * - Landed에서 낙하 시작 Z와 착지 Z 차이로 FallDistance를 계산한다.
 * - SafeFallDistance 미만은 데미지를 주지 않는다.
 * - SafeFallDistance부터 FatalFallDistance 직전까지 현재 HP 비율 피해를 준다.
 * - FatalFallDistance 이상이면 낙사로 처리한다.
 * - 연출용 FallDamageSuppressionSources가 설정돼있는 곳에 착지하면 거리와 무관하게 낙하 피해를 생략한다.
 *
 * 3. 착지 회복
 * - LandingInputLockMinFallDistance 이상이면 약착지/강착지 공통 착지 잠금을 시작한다.
 * - LandingRecoveryMinFallDistance 이상이면 강착지로 분류한다. 강착지 선택은 LastFallDistance로 계산한다.
 * - 착지 회복 중 입력은 실행하지 않고 마지막 액션 입력 하나만 저장한다.
 * - 이동 입력은 착지 애니메이션 Notify가 CompleteLandingRecoveryAnimation을 호출하기 전까지 반영하지 않는다.
 * - CompleteLandingRecoveryAnimation은 저장된 액션 입력을 즉시 실행한다.
 * - LandingRecoveryAutoFinishDuration은 Notify 누락 시 착지 잠금이 stuck되지 않게 닫는 fallback이다.
 * - LandingRecoveryCameraShakeClass가 비어 있으면 C++ 기본 셰이크를 재생한다.
 * - BP는 OnFallStarted, OnLandedFromFall, OnLandingRecoveryStarted, OnLandingRecoveryEnded에서 연출을 붙인다.
 */

namespace
{
	const FName DefaultFallDamageSuppressionSource(TEXT("Default"));

	FName ResolveFallDamageSuppressionSource(const FName Source)
	{
		return Source.IsNone() ? DefaultFallDamageSuppressionSource : Source;
	}
}

void ABAPlayerCharacter::Falling()
{
	Super::Falling();

	if (!ShouldTrackFall())
	{
		return;
	}

	BeginFallTracking();
}

void ABAPlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!bFallTrackingActive)
	{
		return;
	}

	EndFallTrackingFromLanding();
}

void ABAPlayerCharacter::ResolveInitialGroundedMovementMode()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || !MovementComponent->IsFalling())
	{
		return;
	}

	FFindFloorResult FloorResult;
	MovementComponent->FindFloor(GetActorLocation(), FloorResult, false);
	if (!FloorResult.IsWalkableFloor())
	{
		return;
	}

	bFallTrackingActive = false;
	bFallStartNotified = false;
	LastFallDistance = 0.f;
	MovementComponent->SetMovementMode(MOVE_Walking);
}

bool ABAPlayerCharacter::IsLandingRecoveryActive() const
{
	return bLandingRecoveryActive;
}

bool ABAPlayerCharacter::ShouldPlayHeavyLanding() const
{
	return bLandingRecoveryActive && LastFallDistance >= LandingRecoveryMinFallDistance;
}

bool ABAPlayerCharacter::IsLandingRecoveryInputLocked() const
{
	return bLandingRecoveryInputLocked;
}

void ABAPlayerCharacter::CompleteLandingRecoveryAnimation()
{
	if (!bLandingRecoveryActive)
	{
		return;
	}

	FinishLandingRecovery();
}

bool ABAPlayerCharacter::ResolveLandingRecoveryBeforeAction(const EActionCommand Command, const EActionDirection Direction)
{
	if (!bLandingRecoveryActive)
	{
		return true;
	}

	QueueLandingRecoveryAction(Command, Direction);
	return false;
}

float ABAPlayerCharacter::GetLastFallDistance() const
{
	return LastFallDistance;
}

bool ABAPlayerCharacter::IsFallDamageSuppressed() const
{
	return !FallDamageSuppressionSources.IsEmpty();
}

void ABAPlayerCharacter::SetFallDamageSuppressed(const bool bSuppressed, const FName Source)
{
	const FName SafeSource = ResolveFallDamageSuppressionSource(Source);
	if (bSuppressed)
	{
		FallDamageSuppressionSources.Add(SafeSource);
		return;
	}

	FallDamageSuppressionSources.Remove(SafeSource);
}

void ABAPlayerCharacter::ClearFallDamageSuppression()
{
	FallDamageSuppressionSources.Reset();
}

bool ABAPlayerCharacter::ShouldTrackFall() const
{
	return IsAlive()
		&& !IsOnLadder()
		&& !IsDamageReacting()
		&& !bKnockDownGetUpInProgress
		&& BAPlayerState != EBAPlayerState::Dead
		&& BAPlayerState != EBAPlayerState::Respawning;
}

void ABAPlayerCharacter::BeginFallTracking()
{
	bFallTrackingActive = true;
	bFallStartNotified = false;
	LastFallDistance = 0.f;
	FallStartZ = GetActorLocation().Z;
	ResetLandingRecovery();

	ClearMovementPhase();
}

void ABAPlayerCharacter::UpdateFallLoopNotification(const float /*DeltaTime*/)
{
	if (!bFallTrackingActive || bFallStartNotified)
	{
		return;
	}

	const float CurrentFallDistance = FMath::Max(0.f, FallStartZ - GetActorLocation().Z);
	if (CurrentFallDistance < FMath::Max(0.f, FallLoopStartMinDistance))
	{
		return;
	}

	bFallStartNotified = true;
	K2_OnFallStarted();
}

void ABAPlayerCharacter::EndFallTrackingFromLanding()
{
	bFallTrackingActive = false;
	bFallStartNotified = false;
	LastFallDistance = FMath::Max(0.f, FallStartZ - GetActorLocation().Z);

	bool bFatalFall = false;
	const float AppliedDamage = ApplyFallDamage(LastFallDistance, bFatalFall);

	if (!IsAlive() || bFatalFall)
	{
		K2_OnLandedFromFall(LastFallDistance, AppliedDamage, bFatalFall);
		return;
	}

	BeginLandingRecovery(LastFallDistance);
	K2_OnLandedFromFall(LastFallDistance, AppliedDamage, bFatalFall);
}

float ABAPlayerCharacter::CalculateFallDamage(const float FallDistance) const
{
	if (!StatComponent)
	{
		return 0.f;
	}

	const float DamageStartDistance = FMath::Max(0.f, SafeFallDistance);
	if (FallDistance < DamageStartDistance)
	{
		return 0.f;
	}

	const float FatalDistance = FMath::Max(DamageStartDistance, FatalFallDistance);
	const float DamageRange = FatalDistance - DamageStartDistance;
	const float DamageAlpha = DamageRange > 0.f
		? FMath::Clamp((FallDistance - DamageStartDistance) / DamageRange, 0.f, 1.f)
		: 1.f;
	const float DamagePercent = FMath::Lerp(
		FMath::Clamp(FallDamageMinCurrentHPPercent, 0.f, 100.f),
		FMath::Clamp(FallDamageMaxCurrentHPPercent, 0.f, 100.f),
		DamageAlpha);

	return StatComponent->GetCurrentHP() * DamagePercent / 100.f;
}

float ABAPlayerCharacter::ApplyFallDamage(const float FallDistance, bool& bOutFatalFall)
{
	bOutFatalFall = false;
	if (IsFallDamageSuppressed())
	{
		return 0.f;
	}

	bOutFatalFall = FatalFallDistance > 0.f && FallDistance >= FatalFallDistance;
	if (!StatComponent || !CanReceiveDamage())
	{
		return 0.f;
	}

	LastDamageReactionType = EBADamageReactionType::HitReact;
	LastDamageHitDirection = EActionDirection::Any;
	LastDamageDirection = FVector::DownVector;
	LastDamageLaunchHorizontalSpeed = 0.f;
	LastDamageLaunchVerticalSpeed = 0.f;

	const float PreviousHP = StatComponent->GetCurrentHP();
	const float FallDamage = bOutFatalFall
		? StatComponent->GetCurrentHP()
		: CalculateFallDamage(FallDistance);
	if (FallDamage <= 0.f)
	{
		return 0.f;
	}

	StatComponent->ApplyDamage(FallDamage + StatComponent->GetDefence());
	return FMath::Max(0.f, PreviousHP - StatComponent->GetCurrentHP());
}

void ABAPlayerCharacter::BeginLandingRecovery(const float FallDistance)
{
	const bool bShouldPlayHeavyLanding = FallDistance >= LandingRecoveryMinFallDistance;
	if (!bShouldPlayHeavyLanding && FallDistance < LandingInputLockMinFallDistance)
	{
		return;
	}

	bLandingRecoveryActive = true;
	bLandingRecoveryInputLocked = true;
	ClearQueuedLandingRecoveryAction();
	SetMoveInputVector(FVector2D::ZeroVector);

	if (ActionComponent)
	{
		ActionComponent->CancelCurrentAction();
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	MovementRuntime.Phase = EPlayerMovementPhase::None;
	MovementRuntime.PhaseElapsedTime = 0.f;
	MovementRuntime.bWaitingForPhaseAnimation = false;

	K2_OnLandingRecoveryStarted(FallDistance, nullptr);
	if (bShouldPlayHeavyLanding)
	{
		PlayLandingRecoveryCameraShake();
	}

	const float AutoFinishDuration = FMath::Max(0.f, LandingRecoveryAutoFinishDuration);
	if (AutoFinishDuration <= 0.f)
	{
		FinishLandingRecovery();
		return;
	}
	GetWorldTimerManager().SetTimer(
		LandingRecoveryTimerHandle,
		this,
		&ABAPlayerCharacter::FinishLandingRecovery,
		AutoFinishDuration,
		false);
}

void ABAPlayerCharacter::FinishLandingRecovery()
{
	EndLandingRecovery(true);
}

void ABAPlayerCharacter::EndLandingRecovery(const bool bStartQueuedAction)
{
	const bool bWasLandingRecoveryActive = bLandingRecoveryActive;
	if (!bLandingRecoveryActive && !bLandingRecoveryInputLocked)
	{
		ClearQueuedLandingRecoveryAction();
		return;
	}

	const EActionCommand QueuedCommand = bStartQueuedAction ? LandingRecoveryQueuedCommand : EActionCommand::None;
	const EActionDirection QueuedDirection = LandingRecoveryQueuedDirection;
	ClearQueuedLandingRecoveryAction();

	bLandingRecoveryActive = false;
	bLandingRecoveryInputLocked = false;
	GetWorldTimerManager().ClearTimer(LandingRecoveryTimerHandle);

	if (bWasLandingRecoveryActive)
	{
		K2_OnLandingRecoveryEnded();
	}

	StartQueuedLandingRecoveryAction(QueuedCommand, QueuedDirection);
}

void ABAPlayerCharacter::ResetLandingRecovery()
{
	GetWorldTimerManager().ClearTimer(LandingRecoveryTimerHandle);
	bLandingRecoveryActive = false;
	bLandingRecoveryInputLocked = false;
	if (!bFallTrackingActive)
	{
		bFallStartNotified = false;
	}
	ClearQueuedLandingRecoveryAction();
}

void ABAPlayerCharacter::ClearQueuedLandingRecoveryAction()
{
	LandingRecoveryQueuedCommand = EActionCommand::None;
	LandingRecoveryQueuedDirection = EActionDirection::Any;
}

void ABAPlayerCharacter::QueueLandingRecoveryAction(const EActionCommand Command, const EActionDirection Direction)
{
	LandingRecoveryQueuedCommand = Command;
	LandingRecoveryQueuedDirection = Direction;
}

void ABAPlayerCharacter::StartQueuedLandingRecoveryAction(const EActionCommand Command, const EActionDirection Direction)
{
	switch (Command)
	{
	case EActionCommand::LightAttack:
	case EActionCommand::HeavyAttack:
		TryAttack(Command);
		break;
	case EActionCommand::Guard:
		if (bGuardInputHeld)
		{
			TryStartGuard();
		}
		break;
	case EActionCommand::Dodge:
	case EActionCommand::UseConsumable:
		if (ActionComponent)
		{
			ActionComponent->TryStartAction(Command, Direction);
		}
		break;
	case EActionCommand::None:
	default:
		break;
	}
}
