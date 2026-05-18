#include "Player/BAPlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

// 컨트롤러가 요청한 보행 속도를 저장한다. 실제 적용은 공통 Movement 업데이트에서 결정한다.
void ABAPlayerCharacter::SetMovementState(const EMovementState NewState)
{
	MovementRuntime.DesiredGait = NewState;
}

// 2D 이동 입력을 저장한다. 입력의 소비는 캐릭터 Tick에서 일괄 처리한다.
void ABAPlayerCharacter::SetMoveInputVector(const FVector2D& NewMoveInput)
{
	MovementRuntime.MoveInputVector = NewMoveInput;
	if (MovementRuntime.MoveInputVector.SizeSquared() > 1.f)
	{
		MovementRuntime.MoveInputVector.Normalize();
	}

	SetHasMoveInput(!MovementRuntime.MoveInputVector.IsNearlyZero());
}

// 이동 입력 유무를 설정하고 입력이 끊긴 경우 원본 입력을 초기화한다.
void ABAPlayerCharacter::SetHasMoveInput(const bool bNewHasMoveInput)
{
	MovementRuntime.bHasMoveInput = bNewHasMoveInput;
	if (!MovementRuntime.bHasMoveInput)
	{
		MovementRuntime.MoveInputVector = FVector2D::ZeroVector;
	}
}

// Free/Strafe 제어 방식을 변경한다.
void ABAPlayerCharacter::SetLocomotionMode(const EPlayerLocomotionMode NewMode)
{
	if (MovementRuntime.LocomotionMode == NewMode)
	{
		return;
	}

	MovementRuntime.LocomotionMode = NewMode;
	SyncFreeStrafeFacingMode();
}

// 전투 모드(무기 파지 상태)를 설정한다.
void ABAPlayerCharacter::SetCombatMode(const EPlayerCombatMode NewMode)
{
	MovementRuntime.CombatMode = NewMode;
}

// 현재 실제 적용 중인 Walk/Run/Sprint를 반환한다.
EMovementState ABAPlayerCharacter::GetMovementState() const
{
	return MovementRuntime.ActiveGait;
}

// 컨트롤러 입력이 요청한 Walk/Run/Sprint를 반환한다.
EMovementState ABAPlayerCharacter::GetDesiredMovementState() const
{
	return MovementRuntime.DesiredGait;
}

// 현재 Free/Strafe 제어 방식을 반환한다.
EPlayerLocomotionMode ABAPlayerCharacter::GetLocomotionMode() const
{
	return MovementRuntime.LocomotionMode;
}

// 현재 공통 Movement Phase를 반환한다.
EPlayerMovementPhase ABAPlayerCharacter::GetMovementPhase() const
{
	return MovementRuntime.Phase;
}

// 현재 전투 모드를 반환한다.
EPlayerCombatMode ABAPlayerCharacter::GetCombatMode() const
{
	return MovementRuntime.CombatMode;
}

// 현재 이동 입력이 존재하는지 반환한다.
bool ABAPlayerCharacter::HasMoveInput() const
{
	return MovementRuntime.bHasMoveInput;
}

// 저장된 원본 2D 이동 입력을 반환한다.
FVector2D ABAPlayerCharacter::GetMoveInputVector() const
{
	return MovementRuntime.MoveInputVector;
}

// 현재 이동 입력을 컨트롤 yaw 기준 월드 방향으로 변환한다.
FVector ABAPlayerCharacter::GetMoveInputWorldDirection() const
{
	if (!MovementRuntime.bHasMoveInput || IsActionMovementLocked())
	{
		return FVector::ZeroVector;
	}

	if (MovementRuntime.Phase != EPlayerMovementPhase::Loop && IsMovementPhaseUsingRootMotion())
	{
		return FVector::ZeroVector;
	}

	return ConvertMoveInputToWorldDirection(GetInterpolatedMoveInputVector());
}

// 월드 이동 입력을 캐릭터 로컬 방향으로 변환한다.
FVector ABAPlayerCharacter::GetMoveInputLocalDirection() const
{
	const FVector WorldDirection = GetMoveInputWorldDirection();
	if (WorldDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	return GetActorTransform().InverseTransformVectorNoScale(WorldDirection).GetSafeNormal();
}

// AnimBP 호환용 방향 값. 실제 속도 기준의 캐릭터 로컬 각도를 반환한다.
float ABAPlayerCharacter::GetMoveInputDirectionAngle() const
{
	return GetVelocityDirectionAngle();
}

// 캐릭터 로컬 기준 현재 속도 방향을 각도로 반환한다.
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

// Z축을 제외한 지면 이동 속도를 반환한다.
float ABAPlayerCharacter::GetGroundSpeed() const
{
	const FVector Velocity = GetVelocity();
	return FVector(Velocity.X, Velocity.Y, 0.f).Size();
}

// Start/Stop/Turn 진입 시 고정한 방향 각도를 반환한다.
float ABAPlayerCharacter::GetMovementPhaseDirectionAngle() const
{
	return MovementRuntime.PhaseEntryLocalAngle;
}

// 현재 Movement Phase가 지정한 값인지 반환한다.
bool ABAPlayerCharacter::IsMovementPhase(const EPlayerMovementPhase Phase) const
{
	return MovementRuntime.Phase == Phase;
}

// 현재 Phase가 루트모션 소유인지 반환한다.
bool ABAPlayerCharacter::IsMovementPhaseUsingRootMotion() const
{
	return DoesGaitPhaseUseRootMotion(MovementRuntime.Phase, MovementRuntime.ActiveGait);
}

// ABP Notify가 현재 Phase 애니메이션 종료를 알릴 때 호출한다.
void ABAPlayerCharacter::CompleteMovementPhaseAnimation(const EPlayerMovementPhase CompletedPhase)
{
	if (MovementRuntime.Phase != CompletedPhase)
	{
		return;
	}

	FinishCurrentMovementPhase();
}

// Movement 전체를 갱신한다.
void ABAPlayerCharacter::TickMovementRuntime(const float DeltaTime)
{
	UnlockSprintAfterRecovery();
	UpdateInterpolatedMoveInputDirection(DeltaTime);
	UpdatePhaseFromInputAndGait(DeltaTime);
	SyncFreeStrafeFacingMode();
	UpdateInterpolatedFacingRotation();
	ApplyBufferedMoveInput();
	DrainSprintStaminaDuringLoop(DeltaTime);
}

// 현재 입력과 요청 Gait를 기준으로 공통 Phase를 갱신한다.
void ABAPlayerCharacter::UpdatePhaseFromInputAndGait(const float DeltaTime)
{
	MovementRuntime.PhaseElapsedTime += DeltaTime;

	const EMovementState AllowedGait = GetStaminaAllowedGait(MovementRuntime.DesiredGait);
	if (AllowedGait != MovementRuntime.ActiveGait)
	{
		SetActiveGaitAndSpeed(AllowedGait);
	}

	if (!MovementRuntime.bHasMoveInput)
	{
		if (MovementRuntime.Phase == EPlayerMovementPhase::None)
		{
			return;
		}

		if (MovementRuntime.Phase != EPlayerMovementPhase::Stop && IsPhaseEnabledForGait(EPlayerMovementPhase::Stop, MovementRuntime.ActiveGait))
		{
			BeginMovementPhase(EPlayerMovementPhase::Stop);
			return;
		}

		if (MovementRuntime.Phase == EPlayerMovementPhase::Stop && MovementRuntime.bWaitingForPhaseAnimation)
		{
			return;
		}

		BeginMovementPhase(EPlayerMovementPhase::None);
		return;
	}

	if (MovementRuntime.Phase == EPlayerMovementPhase::None || MovementRuntime.Phase == EPlayerMovementPhase::Stop)
	{
		BeginMovementPhase(IsPhaseEnabledForGait(EPlayerMovementPhase::Start, MovementRuntime.ActiveGait)
			? EPlayerMovementPhase::Start
			: EPlayerMovementPhase::Loop);
		return;
	}

	if (MovementRuntime.bWaitingForPhaseAnimation)
	{
		return;
	}

	if (MovementRuntime.Phase == EPlayerMovementPhase::Start || MovementRuntime.Phase == EPlayerMovementPhase::Turn)
	{
		FinishCurrentMovementPhase();
		return;
	}

	if (MovementRuntime.Phase == EPlayerMovementPhase::Loop && ShouldEnterTurnPhase())
	{
		BeginMovementPhase(EPlayerMovementPhase::Turn);
	}
}

// 공통 Movement Phase에 진입하고 진입 방향을 고정한다.
void ABAPlayerCharacter::BeginMovementPhase(const EPlayerMovementPhase NewPhase)
{
	if (MovementRuntime.Phase == NewPhase)
	{
		return;
	}

	MovementRuntime.Phase = NewPhase;
	MovementRuntime.PhaseElapsedTime = 0.f;
	MovementRuntime.PhaseEntryInputVector = MovementRuntime.MoveInputVector.GetSafeNormal();
	MovementRuntime.PhaseEntryWorldDirection = ConvertMoveInputToWorldDirection(MovementRuntime.PhaseEntryInputVector);
	MovementRuntime.PhaseEntryLocalAngle = 0.f;

	if (!MovementRuntime.PhaseEntryWorldDirection.IsNearlyZero())
	{
		const FVector LocalDirection = GetActorTransform().InverseTransformVectorNoScale(MovementRuntime.PhaseEntryWorldDirection).GetSafeNormal();
		MovementRuntime.PhaseEntryLocalAngle = FMath::RadiansToDegrees(FMath::Atan2(LocalDirection.Y, LocalDirection.X));
	}

	MovementRuntime.bWaitingForPhaseAnimation = NewPhase != EPlayerMovementPhase::None
		&& NewPhase != EPlayerMovementPhase::Loop
		&& IsPhaseEnabledForGait(NewPhase, MovementRuntime.ActiveGait);

	if (!MovementRuntime.bWaitingForPhaseAnimation && (NewPhase == EPlayerMovementPhase::Start || NewPhase == EPlayerMovementPhase::Turn))
	{
		BeginMovementPhase(EPlayerMovementPhase::Loop);
	}
}

// 현재 Phase를 규칙에 따라 다음 Phase로 완료한다.
void ABAPlayerCharacter::FinishCurrentMovementPhase()
{
	MovementRuntime.bWaitingForPhaseAnimation = false;

	switch (MovementRuntime.Phase)
	{
	case EPlayerMovementPhase::Start:
	case EPlayerMovementPhase::Turn:
		BeginMovementPhase(MovementRuntime.bHasMoveInput ? EPlayerMovementPhase::Loop : EPlayerMovementPhase::None);
		break;
	case EPlayerMovementPhase::Stop:
		BeginMovementPhase(EPlayerMovementPhase::None);
		break;
	case EPlayerMovementPhase::Loop:
	case EPlayerMovementPhase::None:
	default:
		break;
	}
}

// 실제 적용 중인 Gait를 바꾸고 MovementComponent 속도를 갱신한다.
void ABAPlayerCharacter::SetActiveGaitAndSpeed(const EMovementState NewGait)
{
	const EMovementState PreviousGait = MovementRuntime.ActiveGait;
	MovementRuntime.ActiveGait = NewGait;
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = GetSpeedForGait(NewGait);
	}

	if (PreviousGait == NewGait)
	{
		return;
	}

	if (NewGait == EMovementState::Sprint)
	{
		PauseSprintStaminaRecovery();
	}
	else if (PreviousGait == EMovementState::Sprint)
	{
		ResumeSprintStaminaRecovery(true);
	}
}

// Sprint 가능 여부를 반영해 실제 허용 Gait를 반환한다.
EMovementState ABAPlayerCharacter::GetStaminaAllowedGait(const EMovementState RequestedGait) const
{
	if (RequestedGait == EMovementState::Sprint && !IsSprintAllowedByStamina())
	{
		return EMovementState::Run;
	}

	return RequestedGait;
}

// Gait별 Phase 설정을 반환한다.
const FBAPlayerMovementPhaseSettings& ABAPlayerCharacter::GetPhaseSettings(const EMovementState Gait) const
{
	switch (Gait)
	{
	case EMovementState::Walk:
		return GaitSettings.Walk;
	case EMovementState::Sprint:
		return GaitSettings.Sprint;
	case EMovementState::Run:
	default:
		return GaitSettings.Run;
	}
}

// Gait별 이동 속도를 반환한다.
float ABAPlayerCharacter::GetSpeedForGait(const EMovementState Gait) const
{
	switch (Gait)
	{
	case EMovementState::Walk:
		return SpeedSettings.WalkSpeed;
	case EMovementState::Sprint:
		return SpeedSettings.SprintSpeed;
	case EMovementState::Run:
	default:
		return SpeedSettings.RunSpeed;
	}
}

// Gait별로 지정 Phase를 사용할지 반환한다.
bool ABAPlayerCharacter::IsPhaseEnabledForGait(const EPlayerMovementPhase Phase, const EMovementState Gait) const
{
	const FBAPlayerMovementPhaseSettings& Settings = GetPhaseSettings(Gait);
	switch (Phase)
	{
	case EPlayerMovementPhase::Start:
		return Settings.bUseStart;
	case EPlayerMovementPhase::Stop:
		return Settings.bUseStop;
	case EPlayerMovementPhase::Turn:
		return Settings.bUseTurn;
	case EPlayerMovementPhase::Loop:
	case EPlayerMovementPhase::None:
	default:
		return false;
	}
}

// 지정 Phase가 루트모션 소유인지 반환한다.
bool ABAPlayerCharacter::DoesGaitPhaseUseRootMotion(const EPlayerMovementPhase Phase, const EMovementState Gait) const
{
	const FBAPlayerMovementPhaseSettings& Settings = GetPhaseSettings(Gait);
	switch (Phase)
	{
	case EPlayerMovementPhase::Start:
		return Settings.bUseStart && Settings.bStartUsesRootMotion;
	case EPlayerMovementPhase::Stop:
		return Settings.bUseStop && Settings.bStopUsesRootMotion;
	case EPlayerMovementPhase::Turn:
		return Settings.bUseTurn && Settings.bTurnUsesRootMotion;
	case EPlayerMovementPhase::Loop:
	case EPlayerMovementPhase::None:
	default:
		return false;
	}
}

// 현재 Loop에서 Turn Phase를 요청할지 검사한다.
bool ABAPlayerCharacter::ShouldEnterTurnPhase() const
{
	if (!MovementRuntime.bHasMoveInput || !IsPhaseEnabledForGait(EPlayerMovementPhase::Turn, MovementRuntime.ActiveGait))
	{
		return false;
	}

	const FBAPlayerMovementPhaseSettings& Settings = GetPhaseSettings(MovementRuntime.ActiveGait);
	return GetGroundSpeed() >= Settings.TurnMinSpeed
		&& FMath::Abs(CalculateInputYawDeltaFromActor()) >= Settings.TurnMinAngle;
}

// 캐릭터 방향과 입력 방향 사이의 signed yaw 차이를 계산한다.
float ABAPlayerCharacter::CalculateInputYawDeltaFromActor() const
{
	const FVector DesiredDirection = ConvertMoveInputToWorldDirection(MovementRuntime.MoveInputVector);
	if (DesiredDirection.IsNearlyZero())
	{
		return 0.f;
	}

	return FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredDirection.Rotation().Yaw);
}
