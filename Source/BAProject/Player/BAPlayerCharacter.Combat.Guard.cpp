#include "Player/BAPlayerCharacter.h"

#include "Component/ActionComponent.h"
#include "Component/ActionAnimationComponent.h"
#include "Component/StatComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

/*
 * Guard Policy Summary
 *
 * 가드는 "입력 유지", "실제 방어 가능 구간", "피격 리액션"을 분리해서 다룬다.
 * 입력을 누르고 있다는 사실만으로 방어가 성립하지 않고, 몽타주 안의 가드 윈도우가 열려야 방어 판정이 난다.
 *
 * 1. 기본 흐름
 * - 가드 입력을 누르면 Loop 섹션이 바로 재생되고, Loop 섹션을 반복한다.
 * - 평상시 가드 해제는 별도 End 섹션 없이 몽타주 BlendOut으로 locomotion에 복귀한다.
 * - 이동 중 가드를 누르면 하체 이동은 유지하고, 이동 속도는 Walk로 낮춘다.
 * - 가드 윈도우가 열린 동안만 정면 방어가 가능하며, 이때 스태미너 회복 속도도 가드용 배율로 낮아진다.
 * - GuardHit/PerfectGuard 이후에는 입력을 뗀 뒤 짧은 유지 시간을 허용한다.
 *
 * 2. 상태 의미
 * - Guarding: 방어 자세가 실제로 성립한 상태다.
 * - Blocking: 일반 가드로 공격을 막은 직후다. GuardHit 중에도 추가 공격을 막을 수 있게 유지된다.
 * - GuardBroken: 스태미너가 부족해서 가드가 깨진 상태다. 이 상태에서는 긴 브레이크 리액션이 끝날 때까지 무방비다.
 *
 * 3. 피격 처리
 * - 퍼펙트 가드가 성공하면 다른 모든 처리보다 우선한다. HP 피해 없이 스태미너만 절반 비용으로 소비하고, 일반 가드와 같은 짧은 넉백을 받는다.
 * - 일반 가드가 성공하면 스태미너를 소비하고, HP 피해는 현재 임시 흡수 배율만큼 줄여서 적용한다.
 * - 스태미너 소비에 실패하면 가드 브레이크가 된다. 피해 흡수는 일반 가드와 같지만, 긴 리액션과 루트모션 밀림이 패널티다.
 * - GuardHit 리액션 중에도 입력이 유지된다면 방어 상태가 유지되며 스테미너나 체력이 있는 한 계속 막을 수 있다.
 * - GuardBreak 리액션 중에는 무방비지만, 리액션이 끝났을 때 입력이 유지되고 조건이 맞으면 가드를 다시 시작한다.
 *
 * 4. 이동과 애니메이션
 * - 이동 입력이 없으면 가드는 풀바디 포즈로 재생된다.
 * - 이동 중 가드를 누르면 하체는 이동을 유지하고 상체만 가드 포즈를 얹는다.
 * - 가드 중 이동은 허용하지만 전투 템포를 위해 Walk 속도로 제한한다.
 * - 가드 이동은 방어 판정과 별개다. Loop가 재생 중이어도 가드 윈도우 전에는 막을 수 없다.
 *
 * 5. 인터럽트 예외
 * - 가드를 끊을 수 있는 액션은 구르기, 공격, 스프린트 같은 특수 행동이다.
 * - 이 행동들은 가드 몽타주를 즉시 BlendOut시키고 자기 액션으로 전환한다.
 * - 가드 브레이크 리액션 중에는 입력 유지 여부와 상관없이 가드를 강제로 종료한다.
 * - 사망, 사다리, 피격 리액션 종료 실패, 스태미너 부족 같은 상황에서는 가드 복귀를 포기하고 상태를 정리한다.
 *
 * 6. 데이터 부채
 * - 일반 가드 피해 감소율은 현재 StatComponent의 기본 스탯으로 임시 관리한다.
 * - 퍼펙트 가드 스태미너 배율은 현재 코드에 임시 고정되어 있다.
 * - 장기적으로는 둘 다 무기/장비 데이터로 분리해야 한다.
 */
namespace
{
	// TODO: 무기 테이블로 분리 필요. 퍼펙트 가드 스태미너 비용 배율도 장비별 정책으로 옮겨야 한다.
	constexpr float PerfectGuardStaminaCostMultiplier = 0.5f;

	bool IsGuardInterruptingActionType(const EActionType ActionType)
	{
		return ActionType == EActionType::DodgeRoll
			|| ActionType == EActionType::LightAttack
			|| ActionType == EActionType::HeavyAttack
			|| ActionType == EActionType::Backstep
			|| ActionType == EActionType::Sprint
			|| ActionType == EActionType::UseConsumable;
	}
}

bool ABAPlayerCharacter::TryStartGuard()
{
	const bool bWasGuardReleaseGraceActive = GetWorldTimerManager().IsTimerActive(GuardReleaseGraceTimerHandle);
	ClearGuardReleaseGrace();
	bGuardInputHeld = true;

	if (!ActionComponent)
	{
		return false;
	}

	if (!ResolveLandingRecoveryBeforeAction(EActionCommand::Guard))
	{
		return false;
	}

	if (IsRecoveryEscapeRequiredForCurrentState())
	{
		if (!CanUseRecoveryEscapeGuard())
		{
			bGuardInputHeld = false;
			return false;
		}

		ExitCurrentRecoveryForEscape(false);
	}

	if (!CanAcceptActionInput())
	{
		bGuardInputHeld = false;
		return false;
	}

	if (bWasGuardReleaseGraceActive && ActionComponent->GetActiveActionType() == EActionType::Guard)
	{
		return true;
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
	bGuardInputHeld = false;
	if (CanUseGuardReleaseGraceAfterSuccess())
	{
		bGuardReleaseGraceAvailable = false;
		ScheduleGuardReleaseGrace();
		return;
	}

	StopGuardImmediately();
}

void ABAPlayerCharacter::StopGuardImmediately()
{
	bGuardInputHeld = false;
	bGuardReleaseGraceAvailable = false;
	ClearGuardReleaseGrace();
	if (LandingRecoveryQueuedCommand == EActionCommand::Guard)
	{
		ClearQueuedLandingRecoveryAction();
	}

	if (!ActionComponent)
	{
		return;
	}

	const EGuardState GuardState = ActionComponent->GetGuardState();
	const bool bGuardActionRunning = ActionComponent->GetActiveActionType() == EActionType::Guard;
	const bool bGuardWindowActive = GuardState == EGuardState::Guarding || GuardState == EGuardState::Blocking;
	const bool bGuardBroken = GuardState == EGuardState::GuardBroken;
	SetGuardWindowActive(false);
	if (bGuardBroken)
	{
		ActionComponent->SetGuardState(EGuardState::None);
	}

	if (bGuardActionRunning)
	{
		// 가드 해제는 별도 End 섹션 없이 현재 몽타주의 BlendOut으로만 처리한다.
		ActionComponent->CompleteCurrentAction();
	}

	if (!bGuardWindowActive && !bGuardBroken)
	{
		return;
	}

	if (IsDamageReacting())
	{
		return;
	}

	SetBAPlayerState(EBAPlayerState::None);
	SetCombatMode(EPlayerCombatMode::None);
}

void ABAPlayerCharacter::CancelGuardForSprintInput()
{
	// 이미 누르고 있던 Sprint 유지 상태와 구분해, 새 Sprint 입력에서만 가드를 특수 행동으로 끊는다.
	CancelGuardForActionInterrupt();
}

void ABAPlayerCharacter::CancelGuardForActionInterrupt()
{
	if (!ActionComponent)
	{
		return;
	}

	const bool bGuardActionRunning = ActionComponent->GetActiveActionType() == EActionType::Guard;
	const EGuardState GuardState = ActionComponent->GetGuardState();
	const bool bHasGuardState = GuardState == EGuardState::Guarding
		|| GuardState == EGuardState::Blocking
		|| GuardState == EGuardState::GuardBroken;
	if (!bGuardInputHeld && !bGuardActionRunning && !bHasGuardState && BAPlayerState != EBAPlayerState::Guarding)
	{
		return;
	}

	// 공격/구르기는 가드 몽타주를 BlendOut시키고 즉시 자기 액션으로 전환한다.
	// 가드 입력 유지 플래그까지 내려야 인터럽트 액션 종료 후 GuardHit 복귀 같은 경로가 재진입하지 않는다.
	bGuardInputHeld = false;
	bGuardReleaseGraceAvailable = false;
	ClearGuardReleaseGrace();
	SetGuardWindowActive(false);
	SetPerfectGuardWindowActive(false);

	if (bGuardActionRunning)
	{
		ActionComponent->CancelCurrentAction();
	}

	ActionComponent->SetGuardState(EGuardState::None);
	if (BAPlayerState == EBAPlayerState::Guarding)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}
	SetCombatMode(EPlayerCombatMode::None);
}

void ABAPlayerCharacter::BindGuardActionCallbacks()
{
	if (ActionComponent)
	{
		ActionComponent->OnActionStarted.AddUniqueDynamic(this, &ABAPlayerCharacter::HandleGuardInterruptingActionStarted);
	}

	if (ActionAnimationComponent)
	{
		ActionAnimationComponent->OnActionMontageEnded.AddUniqueDynamic(this, &ABAPlayerCharacter::HandleGuardActionMontageEnded);
	}
}

void ABAPlayerCharacter::HandleGuardInterruptingActionStarted(
	const int32 /*ActionTid*/,
	const EActionType ActionType)
{
	if (!IsGuardInterruptingActionType(ActionType))
	{
		return;
	}

	CancelGuardForActionInterrupt();
}

void ABAPlayerCharacter::HandleGuardActionMontageEnded(
	const int32 /*ActionTid*/,
	const EActionType ActionType,
	UAnimMontage* /*Montage*/,
	const bool /*bInterrupted*/)
{
	if (ActionType != EActionType::Guard || IsDamageReacting())
	{
		return;
	}

	bGuardReleaseGraceAvailable = false;
	ClearGuardReleaseGrace();
	SetGuardWindowActive(false);
	SetBAPlayerState(EBAPlayerState::None);
	SetCombatMode(EPlayerCombatMode::None);
}

void ABAPlayerCharacter::ScheduleGuardReleaseGrace(const float OverrideDelay)
{
	const float Delay = FMath::Max(0.f, OverrideDelay >= 0.f ? OverrideDelay : GuardReleaseGraceDuration);
	if (Delay <= 0.f)
	{
		StopGuardImmediately();
		return;
	}

	GetWorldTimerManager().ClearTimer(GuardReleaseGraceTimerHandle);
	GetWorldTimerManager().SetTimer(
		GuardReleaseGraceTimerHandle,
		this,
		&ABAPlayerCharacter::StopGuardImmediately,
		Delay,
		false);
}

void ABAPlayerCharacter::ClearGuardReleaseGrace()
{
	GetWorldTimerManager().ClearTimer(GuardReleaseGraceTimerHandle);
}

bool ABAPlayerCharacter::ShouldDelayGuardRelease() const
{
	if (GuardReleaseGraceDuration <= 0.f || !ActionComponent || !IsAlive() || IsOnLadder())
	{
		return false;
	}

	const EGuardState GuardState = ActionComponent->GetGuardState();
	const bool bGuardStateActive = GuardState == EGuardState::Guarding || GuardState == EGuardState::Blocking;
	return ActionComponent->GetActiveActionType() == EActionType::Guard
		|| bGuardStateActive
		|| BAPlayerState == EBAPlayerState::Guarding;
}

bool ABAPlayerCharacter::CanUseGuardReleaseGraceAfterSuccess() const
{
	return bGuardReleaseGraceAvailable && ShouldDelayGuardRelease();
}

void ABAPlayerCharacter::ConfigureGuardMontageSections()
{
	if (!ActionAnimationComponent)
	{
		return;
	}

	ActionAnimationComponent->SetActiveMontageNextSection(GuardLoopSection, GuardLoopSection);
}

void ABAPlayerCharacter::ConsumePerfectGuardStaminaCost()
{
	if (!ActionComponent)
	{
		return;
	}

	// TODO: 무기 테이블로 분리 필요. 퍼펙트 가드 스태미너 비용 배율 적용도 장비 정책에서 계산해야 한다.
	ActionComponent->ConsumeActionStaminaCostByType(EActionType::Guard, GetPerfectGuardStaminaCostMultiplier());
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
			ActionComponent->ApplyActiveActionStaminaRecoveryRateMultiplier();
			ActionComponent->SetGuardState(EGuardState::Guarding);
		}
		return;
	}

	if (ActionComponent->GetActiveActionType() == EActionType::Guard)
	{
		ActionComponent->ClearActiveActionStaminaRecoveryRateMultiplier();
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
	// PerfectWindow는 가드 윈도우보다 먼저 열릴 수 있다.
	// 실제 성공 여부는 CombatComponent/OnDamaged에서 "가드 중 && PerfectWindow"로 판정한다.
	bPerfectGuardWindowActive = bActive;
}

bool ABAPlayerCharacter::IsPerfectGuardWindowActive() const
{
	return bPerfectGuardWindowActive;
}

float ABAPlayerCharacter::GetGuardAbsorptionMultiplier() const
{
	// TODO: 무기 테이블로 분리 필요. 현재는 StatComponent의 임시 가드 피해 감소율을 흡수 배율로 변환한다.
	const float GuardDamageReductionRate = StatComponent ? StatComponent->GetGuardDamageReductionRate() : 0.f;
	return 1.f - FMath::Clamp(GuardDamageReductionRate, 0.f, 1.f);
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

	const bool bConsumedStamina = ActionComponent->ConsumeActionStaminaCostByType(EActionType::Guard);
	ActionComponent->SetGuardState(bConsumedStamina ? EGuardState::Blocking : EGuardState::GuardBroken);
	return bConsumedStamina;
}

void ABAPlayerCharacter::KeepGuardActiveAfterGuardSuccess()
{
	if (!ActionComponent || !ShouldResumeGuardAfterGuardReaction())
	{
		return;
	}

	bGuardReleaseGraceAvailable = true;
	ActionComponent->SetGuardState(EGuardState::Blocking);
	SetBAPlayerState(EBAPlayerState::Guarding);
	SetCombatMode(EPlayerCombatMode::Block);
}

bool ABAPlayerCharacter::ShouldResumeGuardAfterGuardReaction() const
{
	return (bGuardInputHeld || GetWorldTimerManager().IsTimerActive(GuardReleaseGraceTimerHandle))
		&& IsAlive()
		&& !IsOnLadder();
}

bool ABAPlayerCharacter::ResumeGuardAfterGuardReaction()
{
	const bool bResumeFromReleaseGrace = !bGuardInputHeld
		&& GetWorldTimerManager().IsTimerActive(GuardReleaseGraceTimerHandle);
	const float RemainingReleaseGrace = bResumeFromReleaseGrace
		? GetWorldTimerManager().GetTimerRemaining(GuardReleaseGraceTimerHandle)
		: 0.f;

	if (!TryStartGuard())
	{
		return false;
	}

	if (bResumeFromReleaseGrace)
	{
		bGuardInputHeld = false;
		ScheduleGuardReleaseGrace(RemainingReleaseGrace);
	}

	if (ActionAnimationComponent)
	{
		ActionAnimationComponent->JumpActiveMontageToSection(GuardLoopSection);
	}

	SetGuardWindowActive(true);
	SetPerfectGuardWindowActive(false);
	if (ActionComponent)
	{
		ActionComponent->SetGuardState(EGuardState::Guarding);
	}
	SetBAPlayerState(EBAPlayerState::Guarding);
	SetCombatMode(EPlayerCombatMode::Block);
	return true;
}

void ABAPlayerCharacter::ShowGuardJudgementDebugMessage(const FString& Message, const FColor& Color) const
{
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, Color, Message);
	}
#endif
}

void ABAPlayerCharacter::HandlePerfectGuardSucceeded(const FHitResult& HitResult, AActor* DamageCauser)
{
	ConsumePerfectGuardStaminaCost();
	ShowGuardJudgementDebugMessage(TEXT("Perfect Guard"), FColor::Cyan);
	PlayPerfectGuardCameraShake();
	PlayPerfectGuardForceFeedback();
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
