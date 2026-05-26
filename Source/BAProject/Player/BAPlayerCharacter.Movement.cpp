#include "Player/BAPlayerCharacter.h"

#include "Component/ActionAnimationComponent.h"
#include "Component/ActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// 컨트롤러가 요청한 보행 속도를 저장한다. 실제 적용은 공통 Movement 업데이트에서 결정한다.
void ABAPlayerCharacter::SetMovementState(const EMovementState NewState)
{
	MovementRuntime.DesiredGait = NewState;
}

// 2D 이동 입력을 저장한다. 입력의 소비는 캐릭터 Tick에서 일괄 처리한다.
void ABAPlayerCharacter::SetMoveInputVector(const FVector2D& NewMoveInput)
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsDamageReacting() && MovementComponent && MovementComponent->IsFalling())
	{
		MovementRuntime.MoveInputVector = FVector2D::ZeroVector;
		SetHasMoveInput(false);
		return;
	}

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
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementRuntime.bHasMoveInput = bNewHasMoveInput
		&& (IsDamageReacting() || !MovementComponent || !MovementComponent->IsFalling());
	if (!MovementRuntime.bHasMoveInput)
	{
		MovementRuntime.MoveInputVector = FVector2D::ZeroVector;
		return;
	}

	RefreshKnockDownGetUpForMoveInput();
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

// AnimBP가 사용할 Walk/Run/Sprint를 반환한다.
EMovementState ABAPlayerCharacter::GetDesiredMovementState() const
{
	return GetMovementAllowedGait(MovementRuntime.DesiredGait);
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

// 가드 액션 중 이동 입력을 허용할지 반환한다.
// 방어 판정은 GuardWindow에서만 열리지만, 이동 중 가드는 하체 locomotion을 유지한다.
bool ABAPlayerCharacter::CanMoveWhileGuarding() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!ActionComponent
		|| !IsAlive()
		|| IsDamageReacting()
		|| bLandingRecoveryActive
		|| IsOnLadder()
		|| (MovementComponent && MovementComponent->IsFalling()))
	{
		return false;
	}

	if (ActionComponent->GetActiveActionType() != EActionType::Guard)
	{
		return false;
	}

	// 방어 판정은 가드 윈도우가 열릴 때만 유효하다.
	// 이동은 별도 정책이므로, 걷는 중 가드를 누른 경우 Start부터 Walk locomotion을 유지한다.
	// 가드 성공 넉백처럼 입력이 아닌 속도는 상체 가드 이동으로 취급하지 않는다.
	return MovementRuntime.bHasMoveInput
		|| (!MovementRuntime.bSuppressVelocityFacingUntilMoveInput && GetGroundSpeed() > 5.f);
}

// AnimBP에서 GuardFullBody/GuardUpperBody 슬롯을 고르기 위한 포즈 분기 전용 값.
// 이동 중 가드는 Loop 단일 몽타주도 하체 locomotion 위에 상체 가드만 얹혀야 하므로 가드 윈도우 여부와 분리한다.
bool ABAPlayerCharacter::ShouldUseUpperBodyGuardPose() const
{
	if (!ActionComponent || !IsAlive() || IsDamageReacting() || bLandingRecoveryActive || IsOnLadder())
	{
		return false;
	}

	if (ActionComponent->GetActiveActionType() != EActionType::Guard)
	{
		return false;
	}

	// AnimBP 슬롯 선택도 이동 허용과 같은 기준을 쓴다.
	// 걷는 중 가드를 누르면 Start/Loop/End 전부 하체 locomotion 위에 상체 가드만 얹혀야 한다.
	return CanMoveWhileGuarding();
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
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementRuntime.bHasMoveInput
		|| IsActionMovementLocked()
		|| (MovementComponent && MovementComponent->IsFalling()))
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

void ABAPlayerCharacter::BindDodgeActionCallbacks()
{
	if (ActionComponent)
	{
		ActionComponent->OnActionStarted.AddUniqueDynamic(this, &ABAPlayerCharacter::HandleDodgeActionStarted);
		ActionComponent->ResolveBufferedActionDirection.BindUObject(
			this,
			&ABAPlayerCharacter::ResolveBufferedActionDirection);
	}

	if (ActionAnimationComponent)
	{
		ActionAnimationComponent->ResolveActionAnimationDirection.BindUObject(
			this,
			&ABAPlayerCharacter::ResolveActionAnimationDirection);
		ActionAnimationComponent->ResolveActionOrientationDirection.BindUObject(
			this,
			&ABAPlayerCharacter::ResolveActionOrientationDirection);
		ActionAnimationComponent->OnActionMontageEnded.AddUniqueDynamic(this, &ABAPlayerCharacter::HandleDodgeActionMontageEnded);
	}
}

void ABAPlayerCharacter::HandleDodgeActionStarted(
	const int32 /*ActionTid*/,
	const EActionType ActionType)
{
	if (IsDamageReacting())
	{
		if (ActionComponent)
		{
			ActionComponent->CancelCurrentAction();
		}
		return;
	}

	if (ActionType == EActionType::DodgeRoll || ActionType == EActionType::Backstep)
	{
		SetBAPlayerState(EBAPlayerState::DodgeRolling);
	}
}

void ABAPlayerCharacter::HandleDodgeActionMontageEnded(
	const int32 /*ActionTid*/,
	const EActionType ActionType,
	UAnimMontage* /*Montage*/,
	const bool bInterrupted)
{
	if ((ActionType == EActionType::DodgeRoll || ActionType == EActionType::Backstep)
		&& !bInterrupted
		&& BAPlayerState == EBAPlayerState::DodgeRolling)
	{
		SetBAPlayerState(EBAPlayerState::None);
	}

	if ((ActionType == EActionType::DodgeRoll || ActionType == EActionType::Backstep)
		&& !MovementRuntime.bHasMoveInput)
	{
		MovementRuntime.bSuppressVelocityFacingUntilMoveInput = true;
		SnapInterpolatedMoveInputTo(FVector2D::ZeroVector);
	}

	if (ActionType == EActionType::DodgeRoll || ActionType == EActionType::Backstep)
	{
		ApplyPendingLockOnStrafeMode();
	}
}

// Movement 전체를 갱신한다.
void ABAPlayerCharacter::TickMovementRuntime(const float DeltaTime)
{
	UnlockSprintAfterRecovery();
	UpdateInterpolatedMoveInputDirection(DeltaTime);
	UpdatePhaseFromInputAndGait(DeltaTime);
	UpdateMaxWalkSpeed(DeltaTime);
	SyncFreeStrafeFacingMode();
	UpdateInterpolatedFacingRotation(DeltaTime);
	ApplyBufferedMoveInput();
	DrainSprintStaminaDuringLoop(DeltaTime);
}

// 현재 입력과 요청 Gait를 기준으로 공통 Phase를 갱신한다.
void ABAPlayerCharacter::UpdatePhaseFromInputAndGait(const float DeltaTime)
{
	MovementRuntime.PhaseElapsedTime += DeltaTime;

	if (DamageReactionState == EPlayerDamageReactionState::KnockDown)
	{
		MovementRuntime.Phase = EPlayerMovementPhase::None;
		MovementRuntime.PhaseElapsedTime = 0.f;
		MovementRuntime.bWaitingForPhaseAnimation = false;
		return;
	}

	if (bLandingRecoveryActive)
	{
		MovementRuntime.Phase = EPlayerMovementPhase::None;
		MovementRuntime.PhaseElapsedTime = 0.f;
		MovementRuntime.bWaitingForPhaseAnimation = false;
		return;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent && MovementComponent->IsFalling())
	{
		MovementRuntime.Phase = EPlayerMovementPhase::None;
		MovementRuntime.PhaseElapsedTime = 0.f;
		MovementRuntime.bWaitingForPhaseAnimation = false;
		return;
	}

	const EMovementState AllowedGait = GetMovementAllowedGait(MovementRuntime.DesiredGait);
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
	MovementRuntime.TargetMaxWalkSpeed = GetSpeedForGait(NewGait);

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

// 실제 MaxWalkSpeed는 gait 변경을 바로 따라가지 않고 보간해 Run/Sprint 전환 충격을 줄인다.
void ABAPlayerCharacter::UpdateMaxWalkSpeed(const float DeltaTime)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	const float TargetSpeed = GetSpeedForGait(MovementRuntime.ActiveGait);
	MovementRuntime.TargetMaxWalkSpeed = TargetSpeed;

	if (MovementRuntime.CurrentMaxWalkSpeed <= 0.f || DeltaTime <= 0.f)
	{
		MovementRuntime.CurrentMaxWalkSpeed = TargetSpeed;
		MovementComponent->MaxWalkSpeed = MovementRuntime.CurrentMaxWalkSpeed;
		return;
	}

	if (FMath::IsNearlyEqual(MovementRuntime.CurrentMaxWalkSpeed, TargetSpeed, 1.f))
	{
		MovementRuntime.CurrentMaxWalkSpeed = TargetSpeed;
	}
	else
	{
		const bool bSpeedingUp = TargetSpeed > MovementRuntime.CurrentMaxWalkSpeed;
		const float InterpRate = bSpeedingUp
			? SpeedSettings.SpeedUpInterpRate
			: SpeedSettings.SlowDownInterpRate;
		MovementRuntime.CurrentMaxWalkSpeed = InterpRate <= 0.f
			? TargetSpeed
			: FMath::FInterpTo(MovementRuntime.CurrentMaxWalkSpeed, TargetSpeed, DeltaTime, InterpRate);
	}

	MovementComponent->MaxWalkSpeed = MovementRuntime.CurrentMaxWalkSpeed;
}

// 가드/스태미너 정책을 반영해 실제 허용 Gait를 반환한다.
EMovementState ABAPlayerCharacter::GetMovementAllowedGait(const EMovementState RequestedGait) const
{
	if (CanMoveWhileGuarding())
	{
		// 가드 중 이동은 허용하되 전투 템포를 위해 항상 Walk로 제한한다.
		return EMovementState::Walk;
	}

	if (RequestedGait == EMovementState::Sprint && !IsSprintAllowedByStamina())
	{
		return EMovementState::Run;
	}

	if (ShouldUseAnalogWalkGait(RequestedGait))
	{
		return EMovementState::Walk;
	}

	return RequestedGait;
}

bool ABAPlayerCharacter::ShouldUseAnalogWalkGait(const EMovementState RequestedGait) const
{
	if (RequestedGait != EMovementState::Run || !MovementRuntime.bHasMoveInput)
	{
		return false;
	}

	const float InputSize = MovementRuntime.MoveInputVector.Size();
	return InputSize > KINDA_SMALL_NUMBER && InputSize <= GetAnalogWalkInputThreshold();
}

float ABAPlayerCharacter::GetAnalogWalkInputThreshold() const
{
	return FMath::Clamp(AnalogWalkInputThreshold, 0.05f, 1.f);
}

float ABAPlayerCharacter::GetMoveInputScaleForActiveGait() const
{
	return MovementRuntime.bHasMoveInput ? 1.f : 0.f;
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
