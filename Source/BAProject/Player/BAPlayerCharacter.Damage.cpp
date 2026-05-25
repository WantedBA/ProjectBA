#include "Player/BAPlayerCharacter.h"

#include "Component/ActionComponent.h"
#include "Component/StatComponent.h"

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
 * - KnockDown/Airborne은 런치 후 지면에 닿은 뒤 바로 idle로 돌아가지 않는다.
 * - 처음 0.5초는 입력 탈출을 받지 않고, 0.5초부터 1.5초까지만 이동/구르기 탈출을 받는다.
 * - 이동 입력은 기립 몽타주를 절반만 재생한 뒤 locomotion으로 복귀한다.
 * - 구르기 입력은 기립 몽타주 없이 즉시 구르기로 탈출한다.
 * - 입력 탈출 창을 지나면 0.5초 더 누운 자세를 유지하고, 아무 입력이 없으면 전체 2초 뒤 기립한다.
 * - 사망, 입력 해제, 사다리 상태, 스태미너 부족처럼 복귀 조건을 만족하지 못하면 가드 상태를 정리한다.
 */

void ABAPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (StatComponent)
	{
		StatComponent->OnDead.AddUniqueDynamic(this, &ABAPlayerCharacter::OnDeath);
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
	LastDamageLaunchHorizontalSpeed = BADamageEvent ? BADamageEvent->LaunchHorizontalSpeed : 0.f;
	LastDamageLaunchVerticalSpeed = BADamageEvent ? BADamageEvent->LaunchVerticalSpeed : 0.f;

	const bool bGuarding = BADamageEvent ? BADamageEvent->bVictimGuarding : IsGuardingAgainstDamage(DamageDirection);
	const bool bPerfectGuard = bGuarding && (BADamageEvent
		? BADamageEvent->bVictimPerfectGuard
		: IsPerfectGuardWindowActive());
	if (bPerfectGuard)
	{
		HandlePerfectGuardSucceeded(ResolveDamageHitResult(DamageEvent), DamageCauser);
		KeepGuardActiveAfterGuardSuccess();
		// 가드 성공 넉백 속도가 Free 회전 입력처럼 해석되지 않게 막는다.
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
		LastDamageHitDirection = HitDirection;
		LastDamageReactionType = DamageReactionType;
		LastDamageDirection = DamageDirection;
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
		// GuardHit 재생 중에도 넉백 속도가 캐릭터 회전을 만들지 않게 막는다.
		MovementRuntime.bSuppressVelocityFacingUntilMoveInput = true;
		if (!MovementRuntime.bHasMoveInput)
		{
			SnapInterpolatedMoveInputTo(FVector2D::ZeroVector);
		}
	}
	ApplyDamageReactionKnockback(DamageReactionType, DamageDirection, HitDirection, bGuarding, bGuardBreak);
	PlayDamageReactionAnimation(DamageReactionType, HitDirection, bGuarding, bGuardBreak);
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
	return IsAlive() && !IsDamageReacting() && !bKnockDownGetUpInProgress && !IsOnLadder();
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
