#include "Player/BAPlayerCharacter.h"

#include "Component/StatComponent.h"

// 탈진으로 인해 Sprint 재진입이 잠겨 있는지 반환한다.
bool ABAPlayerCharacter::IsSprintLockedAfterExhausted() const
{
	return SprintRuntime.bLockedAfterExhausted;
}

// Strafe에서 Sprint로 진입한 직후 회전 정책 전환이 잠겨 있는지 반환한다.
bool ABAPlayerCharacter::IsSprintEntryRotationLocked() const
{
	return ShouldKeepStrafeRotationDuringSprintEntry();
}

// 현재 이동 방향과 입력 방향 사이의 Sprint 회전 각도 차이를 반환한다.
float ABAPlayerCharacter::GetSprintTurnDeltaAngle() const
{
	return FMath::FindDeltaAngleDegrees(GetVelocityDirectionAngle(), GetMoveInputDirectionAngle());
}

// AnimBP가 Sprint Stop 애니메이션을 시작해야 하는지 반환한다.
bool ABAPlayerCharacter::IsSprintStopRequested() const
{
	return SprintRuntime.bSprintStopRequested;
}

// Sprint Stop과 그 뒤에 이어질 Turnaround 요청 상태를 모두 초기화한다.
void ABAPlayerCharacter::ClearSprintStopRequest()
{
	const bool bWasSprintStopActive = SprintRuntime.bSprintStopRequested
		|| SprintRuntime.bMovementLockedBySprintStop
		|| TurnaroundRuntime.State != EPlayerTurnaroundState::None;

	SprintRuntime.bSprintStopRequested = false;
	SprintRuntime.bMovementLockedBySprintStop = false;
	SprintRuntime.bShouldTurnaroundAfterSprintStop = false;
	TurnaroundRuntime.State = EPlayerTurnaroundState::None;
	SprintRuntime.StopRequestRemainingTime = 0.f;
	SprintRuntime.bCanRequestStopFromRecentExit = false;
	SprintRuntime.StopRequestWindowRemainingTime = 0.f;
	TurnaroundRuntime.ElapsedTime = 0.f;
	TurnaroundRuntime.AnimationAngle = 0.f;

	if (bWasSprintStopActive)
	{
		ApplyLocomotionMovementPolicy();
	}
}

// Sprint Stop 애니메이션 종료 시 Turnaround로 이어갈지, 이동 락을 풀지 결정한다.
void ABAPlayerCharacter::CompleteSprintStopAnimation()
{
	SprintRuntime.bSprintStopRequested = false;
	SprintRuntime.StopRequestRemainingTime = 0.f;
	SprintRuntime.bCanRequestStopFromRecentExit = false;
	SprintRuntime.StopRequestWindowRemainingTime = 0.f;

	if (TurnaroundRuntime.State == EPlayerTurnaroundState::Playing)
	{
		return;
	}

	if (TurnaroundRuntime.State == EPlayerTurnaroundState::QueuedAfterSprintStop)
	{
		TurnaroundRuntime.AnimationAngle = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, GetControlRotation().Yaw);
		TurnaroundRuntime.State = EPlayerTurnaroundState::ReadyToBeginAfterSprintStop;
		ApplyLocomotionMovementPolicy();
		return;
	}

	SprintRuntime.bMovementLockedBySprintStop = false;
	SprintRuntime.bShouldTurnaroundAfterSprintStop = false;
	ApplyLocomotionMovementPolicy();
}

// Sprint 가능 여부는 데이터 존재, 현재 스태미나, 탈진 후 재시작 조건을 함께 검사한다.
bool ABAPlayerCharacter::CanSprint() const
{
	if (!SprintRuntime.bHasActionData || !StatComponent)
	{
		return false;
	}

	const float CurrentStamina = StatComponent->GetCurrentStamina();
	const float SprintRestartStamina = StatComponent->GetMaxStamina() * SprintRuntime.RestartStaminaPercent / 100.f;

	if (SprintRuntime.bLockedAfterExhausted && CurrentStamina < SprintRestartStamina)
	{
		return false;
	}

	return CurrentStamina >= SprintRuntime.MinRequiredStamina
		&& (SprintRuntime.StaminaCost <= 0.f ||  CurrentStamina > 0.f);
}

// Sprint 이동이 유지될 만큼 이동 입력이 살아 있는지 반환한다.
bool ABAPlayerCharacter::IsSprintMovementActive() const
{
	return MovementRuntime.bHasMoveInput;
}

// Sprint 유지 중에는 Action 데이터의 스태미나 비용 규칙에 따라 스태미나를 소모한다.
void ABAPlayerCharacter::ConsumeSprintStamina(const float DeltaTime)
{
	if (!StatComponent || SprintRuntime.StaminaCost <= 0.f)
	{
		return;
	}

	const float CurrentStamina = StatComponent->GetCurrentStamina();
	const float ConsumeAmount = CalculateSprintStaminaCost(DeltaTime);
	StatComponent->SetCurrentStamina(CurrentStamina - ConsumeAmount);

	LockSprintIfExhausted();
}

// Action 데이터의 비용 타입에 따라 이번 프레임 Sprint 스태미나 소모량을 계산한다.
float ABAPlayerCharacter::CalculateSprintStaminaCost(const float DeltaTime) const
{
	if (!StatComponent)
	{
		return 0.f;
	}

	switch (SprintRuntime.StaminaCostType)
	{
	case EPlayerStaminaCostType::PerSecond:
		return StatComponent->GetMaxStamina() * SprintRuntime.StaminaCost / 100.f * DeltaTime;
	case EPlayerStaminaCostType::Instant:
	default:
		return SprintRuntime.StaminaCost;
	}
}

// 탈진 락은 충분한 스태미나가 회복될 때까지 Sprint 재진입을 막는다.
void ABAPlayerCharacter::LockSprintIfExhausted()
{
	if (!StatComponent || SprintRuntime.StaminaCost <= 0.f)
	{
		return;
	}

	if (StatComponent->GetCurrentStamina() <= 0.f)
	{
		SprintRuntime.bLockedAfterExhausted = true;
	}
}

// 탈진 락이 걸린 상태에서 재시작 기준 스태미나까지 회복됐는지 검사한다.
void ABAPlayerCharacter::UpdateSprintExhaustionLock()
{
	if (!SprintRuntime.bLockedAfterExhausted || !StatComponent)
	{
		return;
	}

	const float SprintRestartStamina = StatComponent->GetMaxStamina() * SprintRuntime.RestartStaminaPercent / 100.f;
	if (StatComponent->GetCurrentStamina() >= SprintRestartStamina)
	{
		SprintRuntime.bLockedAfterExhausted = false;
	}
}

// Strafe 상태에서 Sprint로 진입할 때 일정 시간 동안 회전 정책 전환을 늦춘다.
void ABAPlayerCharacter::UpdateSprintEntryRotation(const float DeltaTime)
{
	if (!SprintRuntime.bKeepStrafeRotationDuringSprintEntry)
	{
		return;
	}

	SprintRuntime.EntryElapsedTime += DeltaTime;

	const float OrientationSpeed = SpeedSettings.SprintSpeed * SprintSettings.StrafeEntryOrientationSpeedRatio;
	const bool bHasBlendedLongEnough = SprintRuntime.EntryElapsedTime >= SprintSettings.StrafeEntryBlendTime;
	const bool bReachedSprintOrientationSpeed = GetGroundSpeed() >= OrientationSpeed;

	if (bHasBlendedLongEnough && bReachedSprintOrientationSpeed)
	{
		SprintRuntime.bKeepStrafeRotationDuringSprintEntry = false;
		ApplyLocomotionMovementPolicy();
	}
}

// 현재 이동 상태와 진입 락 플래그를 함께 보고 Sprint 진입 회전 락 사용 여부를 반환한다.
bool ABAPlayerCharacter::ShouldKeepStrafeRotationDuringSprintEntry() const
{
	return MovementRuntime.MovementState == EMovementState::Sprint && SprintRuntime.bKeepStrafeRotationDuringSprintEntry;
}

// 입력이 끊기거나 Sprint에서 이탈한 직후, 정지 애니메이션 요청 상태로 진입한다.
void ABAPlayerCharacter::RequestSprintStop()
{
	SprintRuntime.bSprintStopRequested = true;
	SprintRuntime.bMovementLockedBySprintStop = true;
	TurnaroundRuntime.State = SprintRuntime.bShouldTurnaroundAfterSprintStop
		? EPlayerTurnaroundState::QueuedAfterSprintStop
		: EPlayerTurnaroundState::None;
	SprintRuntime.bShouldTurnaroundAfterSprintStop = false;
	SprintRuntime.bCanRequestStopFromRecentExit = false;
	SprintRuntime.StopRequestWindowRemainingTime = 0.f;
	SprintRuntime.StopRequestRemainingTime = FMath::Max(0.f, SprintSettings.StopRequestHoldTime);
	ApplyLocomotionMovementPolicy();
}

// Sprint Stop 요청은 AnimBP가 읽을 수 있도록 짧은 시간만 유지한다.
void ABAPlayerCharacter::UpdateSprintStopRequest(const float DeltaTime)
{
	if (!SprintRuntime.bSprintStopRequested)
	{
		return;
	}

	SprintRuntime.StopRequestRemainingTime -= DeltaTime;
	if (SprintRuntime.StopRequestRemainingTime <= 0.f)
	{
		SprintRuntime.bSprintStopRequested = false;
		SprintRuntime.StopRequestRemainingTime = 0.f;
	}
}

// Sprint 종료 직후 아주 짧은 입력 공백도 정지 요청으로 인정하기 위한 유예 시간.
void ABAPlayerCharacter::StartSprintStopRequestWindow()
{
	SprintRuntime.bCanRequestStopFromRecentExit = true;
	SprintRuntime.StopRequestWindowRemainingTime = FMath::Max(0.f, SprintSettings.StopRequestWindowTime);
}

// Sprint 종료 직후 정지 요청 유예 시간을 감소시키고 만료 시 정책을 갱신한다.
void ABAPlayerCharacter::UpdateSprintStopRequestWindow(const float DeltaTime)
{
	if (!SprintRuntime.bCanRequestStopFromRecentExit)
	{
		return;
	}

	SprintRuntime.StopRequestWindowRemainingTime -= DeltaTime;
	if (SprintRuntime.StopRequestWindowRemainingTime <= 0.f)
	{
		SprintRuntime.bCanRequestStopFromRecentExit = false;
		SprintRuntime.StopRequestWindowRemainingTime = 0.f;

		if (!SprintRuntime.bSprintStopRequested)
		{
			ApplyLocomotionMovementPolicy();
		}
	}
}

// 현재 속도와 요청 상태를 기준으로 Sprint Stop 요청 가능 여부를 반환한다.
bool ABAPlayerCharacter::CanRequestSprintStop() const
{
	return !SprintRuntime.bSprintStopRequested && GetGroundSpeed() >= SprintSettings.StopMinSpeed;
}
