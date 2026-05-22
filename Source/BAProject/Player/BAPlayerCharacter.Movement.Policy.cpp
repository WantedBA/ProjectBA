#include "Player/BAPlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

// Free/Strafe 모드에 맞춰 캐릭터가 바라볼 yaw 기준을 동기화한다.
void ABAPlayerCharacter::SyncFreeStrafeFacingMode()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->MaxWalkSpeed = GetSpeedForGait(MovementRuntime.ActiveGait);

	if (MovementRuntime.ActiveGait == EMovementState::Sprint)
	{
		UseMovementDirectionFacing(*MovementComponent);
		return;
	}

	switch (MovementRuntime.LocomotionMode)
	{
	case EPlayerLocomotionMode::Strafe:
		UseControllerYawFacing(*MovementComponent);
		break;
	case EPlayerLocomotionMode::Free:
	default:
		UseMovementDirectionFacing(*MovementComponent);
		break;
	}
}

// 실제 이동 입력과 별개로 가속방향 바라보게 하기
void ABAPlayerCharacter::UseMovementDirectionFacing(UCharacterMovementComponent& MovementComponent)
{
	bUseControllerRotationYaw = false;
	MovementComponent.bOrientRotationToMovement = false;
	MovementComponent.RotationRate = FRotator(0.f, LocomotionSettings.FreeRotationRateYaw, 0.f);
	MovementComponent.MaxAcceleration = LocomotionSettings.FreeMaxAcceleration;
	MovementComponent.BrakingDecelerationWalking = LocomotionSettings.FreeBrakingDecelerationWalking;
	MovementComponent.GroundFriction = LocomotionSettings.FreeGroundFriction;
}

// 이동 시 캐릭터 가속방향 회전 보간 처리
void ABAPlayerCharacter::UpdateInterpolatedFacingRotation()
{
	if (!ShouldUseInterpolatedFacingRotation())
	{
		return;
	}

	const FVector FacingDirection = ConvertMoveInputToWorldDirection(GetInterpolatedMoveInputVector());
	if (FacingDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FRotator(CurrentRotation.Pitch, FacingDirection.Rotation().Yaw, CurrentRotation.Roll));
}

// 캐릭터 가속방향 회전 보간 처리 적용 판단
bool ABAPlayerCharacter::ShouldUseInterpolatedFacingRotation() const
{
	if (MovementRuntime.CombatMode == EPlayerCombatMode::Block
		&& MovementRuntime.LocomotionMode == EPlayerLocomotionMode::Strafe)
	{
		// Strafe 가드는 카메라/컨트롤러 방향을 유지한다.
		// Free 가드는 아래 일반 Free 회전 규칙을 타서 이동 입력 방향을 바라본다.
		return false;
	}

	if (IsActionMovementLocked())
	{
		return false;
	}

	if (MovementRuntime.Phase != EPlayerMovementPhase::Loop && IsMovementPhaseUsingRootMotion())
	{
		return false;
	}

	return MovementRuntime.ActiveGait == EMovementState::Sprint
		|| MovementRuntime.LocomotionMode == EPlayerLocomotionMode::Free;
}

// Strafe는 컨트롤러 yaw를 기준으로 캐릭터 방향을 유지한다.
void ABAPlayerCharacter::UseControllerYawFacing(UCharacterMovementComponent& MovementComponent)
{
	bUseControllerRotationYaw = true;
	MovementComponent.bOrientRotationToMovement = false;
	MovementComponent.RotationRate = FRotator(0.f, LocomotionSettings.StrafeRotationRateYaw, 0.f);
	MovementComponent.MaxAcceleration = LocomotionSettings.StrafeMaxAcceleration;
	MovementComponent.BrakingDecelerationWalking = LocomotionSettings.StrafeBrakingDecelerationWalking;
	MovementComponent.GroundFriction = LocomotionSettings.StrafeGroundFriction;
}
