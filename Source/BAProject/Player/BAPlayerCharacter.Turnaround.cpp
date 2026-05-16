#include "Player/BAPlayerCharacter.h"

// AnimBP가 Turnaround 애니메이션의 방향과 속도를 결정할 때 사용하는 값.
float ABAPlayerCharacter::GetTurnaroundToControlRotationAngle() const
{
	if (!IsTurnaroundRequested())
	{
		return 0.f;
	}

	return TurnaroundRuntime.AnimationAngle;
}

// Turnaround 각도에 따라 애니메이션 재생 속도를 계산한다.
float ABAPlayerCharacter::GetTurnaroundPlayRate() const
{
	if (!IsTurnaroundRequested())
	{
		return 1.f;
	}

	const float AbsAngle = FMath::Abs(TurnaroundRuntime.AnimationAngle);
	const float SafeAngle = FMath::Max(AbsAngle, TurnaroundSettings.PlayRateMinAngle);
	const float RawPlayRate = TurnaroundSettings.PlayRateReferenceAngle / SafeAngle;
	return FMath::Clamp(RawPlayRate, TurnaroundSettings.MinPlayRate, TurnaroundSettings.MaxPlayRate);
}

// Turnaround 애니메이션을 시작할 준비가 됐거나 이미 재생 중인지 반환한다.
bool ABAPlayerCharacter::IsTurnaroundRequested() const
{
	return TurnaroundRuntime.State == EPlayerTurnaroundState::ReadyToBeginAfterSprintStop
		|| TurnaroundRuntime.State == EPlayerTurnaroundState::Playing;
}

// Sprint Stop 이후 Turnaround가 예약되어 있는지 반환한다.
bool ABAPlayerCharacter::IsTurnaroundQueuedAfterSprintStop() const
{
	return TurnaroundRuntime.State == EPlayerTurnaroundState::QueuedAfterSprintStop
		|| TurnaroundRuntime.State == EPlayerTurnaroundState::ReadyToBeginAfterSprintStop;
}

// Sprint Stop 애니메이션이 끝난 뒤 AnimBP가 실제 Turnaround 재생을 시작했음을 알린다.
void ABAPlayerCharacter::BeginTurnaroundAnimation()
{
	if (TurnaroundRuntime.State != EPlayerTurnaroundState::ReadyToBeginAfterSprintStop)
	{
		return;
	}

	TurnaroundRuntime.State = EPlayerTurnaroundState::Playing;
	TurnaroundRuntime.ElapsedTime = 0.f;
	ApplyLocomotionMovementPolicy();
}

// Turnaround 종료 시 이동 락과 후속 전환 상태를 모두 해제한다.
void ABAPlayerCharacter::CompleteTurnaroundAnimation()
{
	TurnaroundRuntime.State = EPlayerTurnaroundState::None;
	SprintRuntime.bMovementLockedBySprintStop = false;
	SprintRuntime.bShouldTurnaroundAfterSprintStop = false;
	TurnaroundRuntime.ElapsedTime = 0.f;
	TurnaroundRuntime.AnimationAngle = 0.f;
	ApplyLocomotionMovementPolicy();
}

// 애니메이션 콜백이 누락돼도 최대 지속 시간이 지나면 Turnaround를 정리한다.
void ABAPlayerCharacter::UpdateTurnaroundRotation(const float DeltaTime)
{
	if (TurnaroundRuntime.State != EPlayerTurnaroundState::Playing)
	{
		return;
	}

	TurnaroundRuntime.ElapsedTime += DeltaTime;

	if (TurnaroundRuntime.ElapsedTime >= TurnaroundSettings.MaxDuration)
	{
		CompleteTurnaroundAnimation();
	}
}
