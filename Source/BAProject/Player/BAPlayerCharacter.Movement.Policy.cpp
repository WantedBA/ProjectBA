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

// Free는 캐릭터가 이동 방향을 바라보게 한다.
void ABAPlayerCharacter::UseMovementDirectionFacing(UCharacterMovementComponent& MovementComponent)
{
	bUseControllerRotationYaw = false;
	MovementComponent.bOrientRotationToMovement = true;
	MovementComponent.RotationRate = FRotator(0.f, LocomotionSettings.FreeRotationRateYaw, 0.f);
	MovementComponent.MaxAcceleration = LocomotionSettings.FreeMaxAcceleration;
	MovementComponent.BrakingDecelerationWalking = LocomotionSettings.FreeBrakingDecelerationWalking;
	MovementComponent.GroundFriction = LocomotionSettings.FreeGroundFriction;
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
