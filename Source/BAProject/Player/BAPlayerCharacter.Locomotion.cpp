#include "Player/BAPlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

// 이동 상태를 설정하고, 속도 및 Sprint 관련 전환 상태를 업데이트한다.
void ABAPlayerCharacter::SetMovementState(EMovementState NewState)
{
	UpdateSprintExhaustionLock();

	if (NewState == EMovementState::Sprint)
	{
		LockSprintIfExhausted();
	}

	if (NewState == EMovementState::Sprint && !CanSprint())
	{
		NewState = EMovementState::Run;
	}

	const EMovementState PreviousMovementState = MovementRuntime.MovementState;
	if (MovementRuntime.MovementState == NewState)
	{
		return;
	}

	MovementRuntime.MovementState = NewState;
	if (MovementRuntime.MovementState == EMovementState::Sprint)
	{
		ClearSprintStopRequest();
		SprintRuntime.bKeepStrafeRotationDuringSprintEntry = MovementRuntime.LocomotionMode == EPlayerLocomotionMode::Strafe;
		SprintRuntime.bShouldTurnaroundAfterSprintStop = MovementRuntime.LocomotionMode == EPlayerLocomotionMode::Strafe;
		SprintRuntime.EntryElapsedTime = 0.f;
	}
	else if (PreviousMovementState == EMovementState::Sprint)
	{
		StartSprintStopRequestWindow();

		if (!MovementRuntime.bHasMoveInput && CanRequestSprintStop())
		{
			RequestSprintStop();
		}

		SprintRuntime.bKeepStrafeRotationDuringSprintEntry = false;
		SprintRuntime.EntryElapsedTime = 0.f;
	}

	// 기본 이동 속도는 Run으로 시작
	float NewSpeed = SpeedSettings.RunSpeed;
	switch (MovementRuntime.MovementState)
	{
	case EMovementState::Walk:
		NewSpeed = SpeedSettings.WalkSpeed;
		break;
	case EMovementState::Run:
		NewSpeed = SpeedSettings.RunSpeed;
		break;
	case EMovementState::Sprint:
		NewSpeed = SpeedSettings.SprintSpeed;
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
	ApplyLocomotionMovementPolicy();
}

// 2D 이동 입력을 보관하고, 필요시 컨트롤 회전 기준의 월드 좌표로 변환한다.
void ABAPlayerCharacter::SetMoveInputVector(const FVector2D& NewMoveInput)
{
	MovementRuntime.MoveInputVector = NewMoveInput;
	if (MovementRuntime.MoveInputVector.SizeSquared() > 1.f)
	{
		MovementRuntime.MoveInputVector.Normalize();
	}

	SetHasMoveInput(!MovementRuntime.MoveInputVector.IsNearlyZero());
}

// 이동 모드(Free/Strafe)를 변경하고 회전 및 가속 정책을 즉시 재적용한다.
void ABAPlayerCharacter::SetLocomotionMode(const EPlayerLocomotionMode NewMode)
{
	if (MovementRuntime.LocomotionMode == NewMode)
	{
		return;
	}

	MovementRuntime.LocomotionMode = NewMode;
	ApplyLocomotionMovementPolicy();
}

// 전투 모드(무기 파지 상태)를 설정한다.
void ABAPlayerCharacter::SetCombatMode(const EPlayerCombatMode NewMode)
{
	MovementRuntime.CombatMode = NewMode;
}

// 현재 이동 상태(Walk/Run/Sprint)를 반환한다.
EMovementState ABAPlayerCharacter::GetMovementState() const
{
	return MovementRuntime.MovementState;
}

// 현재 이동 모드(Free/Strafe)를 반환한다.
EPlayerLocomotionMode ABAPlayerCharacter::GetLocomotionMode() const
{
	return MovementRuntime.LocomotionMode;
}

// 현재 전투 모드(무기 파지 상태)를 반환한다.
EPlayerCombatMode ABAPlayerCharacter::GetCombatMode() const
{
	return MovementRuntime.CombatMode;
}

// 현재 이동 입력이 존재하는지 반환한다.
bool ABAPlayerCharacter::HasMoveInput() const
{
	return MovementRuntime.bHasMoveInput;
}

// 저장된 2D 이동 입력 벡터를 반환한다.
FVector2D ABAPlayerCharacter::GetMoveInputVector() const
{
	return MovementRuntime.MoveInputVector;
}

// 2D 이동 입력을 컨트롤 회전 기준의 월드 좌표로 변환하여 반환한다.
FVector ABAPlayerCharacter::GetMoveInputWorldDirection() const
{
	if (!MovementRuntime.bHasMoveInput)
	{
		return FVector::ZeroVector;
	}

	const FRotator ControlRot = GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	return (Forward * MovementRuntime.MoveInputVector.Y + Right * MovementRuntime.MoveInputVector.X).GetSafeNormal();
}

// 월드 좌표 이동 입력을 캐릭터 로컬 좌표로 변환하여 반환한다.
FVector ABAPlayerCharacter::GetMoveInputLocalDirection() const
{
	const FVector WorldDirection = GetMoveInputWorldDirection();
	if (WorldDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	return GetActorTransform().InverseTransformVectorNoScale(WorldDirection).GetSafeNormal();
}

// 캐릭터 로컬 기준 이동 입력 방향을 애니메이션 재생용 각도로 반환한다.
float ABAPlayerCharacter::GetMoveInputDirectionAngle() const
{
	const FVector LocalDirection = GetMoveInputLocalDirection();
	if (LocalDirection.IsNearlyZero())
	{
		return 0.f;
	}

	return FMath::RadiansToDegrees(FMath::Atan2(LocalDirection.Y, LocalDirection.X));
}

// 캐릭터 로컬 기준 현재 속도 방향을 애니메이션 재생용 각도로 반환한다.
float ABAPlayerCharacter::GetVelocityDirectionAngle() const
{
	FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(GetVelocity());
	LocalVelocity.Z = 0.f;

	if (LocalVelocity.IsNearlyZero())
	{
		return 0.f;
	}

	LocalVelocity.Normalize();
	return FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
}

// Z축 운동을 제외한 지면 이동 속도를 반환한다.
float ABAPlayerCharacter::GetGroundSpeed() const
{
	const FVector Velocity = GetVelocity();
	return FVector(Velocity.X, Velocity.Y, 0.f).Size();
}

// 현재 상태에 따라 CharacterMovementComponent의 회전, 가속, 마찰 정책을 적용한다.
void ABAPlayerCharacter::ApplyLocomotionMovementPolicy()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	// 1. 실제 Turnaround 재생 중에는 캐릭터 회전을 애니메이션에 맡긴다.
	if (TurnaroundRuntime.State == EPlayerTurnaroundState::Playing)
	{
		ApplyTurnaroundMovementPolicy(*MovementComponent);
		return;
	}

	// 2. Sprint 계열 상태에서 Strafe 진입 회전 유지가 끝났다면 Free 이동 정책을 강제한다.
	if (ShouldUseSprintMovementPolicy() && !ShouldKeepStrafeRotationDuringSprintEntry())
	{
		ApplyFreeMovementPolicy(*MovementComponent);
		return;
	}

	// 3. 일반 이동 상태에서는 현재 Locomotion 모드에 맞는 정책을 적용한다.
	switch (MovementRuntime.LocomotionMode)
	{
	case EPlayerLocomotionMode::Strafe:
		ApplyStrafeMovementPolicy(*MovementComponent);
		break;
	case EPlayerLocomotionMode::Free:
	default:
		ApplyFreeMovementPolicy(*MovementComponent);
		break;
	}
}

// Free 이동 정책은 캐릭터가 이동 방향으로 회전하도록 설정한다.
void ABAPlayerCharacter::ApplyFreeMovementPolicy(UCharacterMovementComponent& MovementComponent)
{
	bUseControllerRotationYaw = false;
	MovementComponent.bOrientRotationToMovement = true;
	MovementComponent.RotationRate = FRotator(0.f, LocomotionSettings.FreeRotationRateYaw, 0.f);
	MovementComponent.MaxAcceleration = LocomotionSettings.FreeMaxAcceleration;
	MovementComponent.BrakingDecelerationWalking = LocomotionSettings.FreeBrakingDecelerationWalking;
	MovementComponent.GroundFriction = LocomotionSettings.FreeGroundFriction;
}

// Strafe 이동 정책은 컨트롤러 yaw를 기준으로 캐릭터 방향을 유지한다.
void ABAPlayerCharacter::ApplyStrafeMovementPolicy(UCharacterMovementComponent& MovementComponent)
{
	bUseControllerRotationYaw = true;
	MovementComponent.bOrientRotationToMovement = false;
	MovementComponent.RotationRate = FRotator(0.f, LocomotionSettings.StrafeRotationRateYaw, 0.f);
	MovementComponent.MaxAcceleration = LocomotionSettings.StrafeMaxAcceleration;
	MovementComponent.BrakingDecelerationWalking = LocomotionSettings.StrafeBrakingDecelerationWalking;
	MovementComponent.GroundFriction = LocomotionSettings.StrafeGroundFriction;
}

// Turnaround 재생 중에는 이동 회전을 막고 Strafe 계열 가속/마찰만 유지한다.
void ABAPlayerCharacter::ApplyTurnaroundMovementPolicy(UCharacterMovementComponent& MovementComponent)
{
	bUseControllerRotationYaw = false;
	MovementComponent.bOrientRotationToMovement = false;
	MovementComponent.MaxAcceleration = LocomotionSettings.StrafeMaxAcceleration;
	MovementComponent.BrakingDecelerationWalking = LocomotionSettings.StrafeBrakingDecelerationWalking;
	MovementComponent.GroundFriction = LocomotionSettings.StrafeGroundFriction;
}

// 이동 입력 유무 변화에 따라 Sprint 정지 또는 Turnaround를 자동으로 처리한다.
void ABAPlayerCharacter::SetHasMoveInput(const bool bNewHasMoveInput)
{
	MovementRuntime.bHasMoveInput = bNewHasMoveInput;
	if (MovementRuntime.bHasMoveInput && (SprintRuntime.bSprintStopRequested
		|| SprintRuntime.bMovementLockedBySprintStop
		|| TurnaroundRuntime.State != EPlayerTurnaroundState::None
		|| IsTurnaroundRequested()))
	{
		ClearSprintStopRequest();
	}

	if (!MovementRuntime.bHasMoveInput)
	{
		MovementRuntime.MoveInputVector = FVector2D::ZeroVector;
	}

	if (!MovementRuntime.bHasMoveInput && MovementRuntime.MovementState == EMovementState::Sprint)
	{
		SetMovementState(EMovementState::Run);
	}
	else if (!MovementRuntime.bHasMoveInput && SprintRuntime.bCanRequestStopFromRecentExit && CanRequestSprintStop())
	{
		RequestSprintStop();
	}
}

// Sprint 상태 및 Sprint 종료 후속 동작 중에 Free 이동 정책을 유지해야 하는지 판단한다.
bool ABAPlayerCharacter::ShouldUseSprintMovementPolicy() const
{
	return MovementRuntime.MovementState == EMovementState::Sprint
		|| SprintRuntime.bSprintStopRequested
		|| SprintRuntime.bMovementLockedBySprintStop
		|| TurnaroundRuntime.State == EPlayerTurnaroundState::QueuedAfterSprintStop
		|| TurnaroundRuntime.State == EPlayerTurnaroundState::ReadyToBeginAfterSprintStop
		|| SprintRuntime.bCanRequestStopFromRecentExit;
}
