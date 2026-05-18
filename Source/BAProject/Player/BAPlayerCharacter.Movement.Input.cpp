#include "Player/BAPlayerCharacter.h"

// Phase가 루트모션 소유가 아닐 때만 저장된 이동 입력을 실제 이동에 사용한다.
void ABAPlayerCharacter::ApplyBufferedMoveInput()
{
	if (!MovementRuntime.bHasMoveInput || IsMovementPhaseUsingRootMotion())
	{
		return;
	}

	const FVector MoveDirection = ConvertMoveInputToWorldDirection(MovementRuntime.MoveInputVector);
	if (!MoveDirection.IsNearlyZero())
	{
		AddMovementInput(MoveDirection, MovementRuntime.MoveInputVector.Size());
	}
}

// 루트모션 Phase가 아닐 때 원본 입력 방향을 보간 입력 방향으로 회전시킨다.
void ABAPlayerCharacter::UpdateInterpolatedMoveInputDirection(const float DeltaTime)
{
	if (IsMovementPhaseUsingRootMotion())
	{
		SnapInterpolatedMoveInputTo(MovementRuntime.MoveInputVector);
		return;
	}

	if (!MovementRuntime.bHasMoveInput)
	{
		const FVector2D VelocityInput = ConvertWorldDirectionToMoveInput(GetVelocity());
		if (!VelocityInput.IsNearlyZero())
		{
			MovementRuntime.InterpolatedMoveInputVector = VelocityInput;
			MovementRuntime.bHasInterpolatedMoveInput = true;
			MovementRuntime.InterpolatedMoveInputMemoryRemainingTime = LocomotionSettings.MoveInputDirectionMemoryTime;
			return;
		}

		if (MovementRuntime.bHasInterpolatedMoveInput)
		{
			MovementRuntime.InterpolatedMoveInputMemoryRemainingTime -= DeltaTime;
			if (MovementRuntime.InterpolatedMoveInputMemoryRemainingTime <= 0.f)
			{
				SnapInterpolatedMoveInputTo(FVector2D::ZeroVector);
			}
		}
		return;
	}

	const FVector2D TargetInput = MovementRuntime.MoveInputVector.GetSafeNormal();
	if (TargetInput.IsNearlyZero())
	{
		return;
	}

	MovementRuntime.InterpolatedMoveInputMemoryRemainingTime = LocomotionSettings.MoveInputDirectionMemoryTime;

	if (!MovementRuntime.bHasInterpolatedMoveInput || MovementRuntime.InterpolatedMoveInputVector.IsNearlyZero())
	{
		SnapInterpolatedMoveInputTo(TargetInput);
		return;
	}

	const FVector2D CurrentInput = MovementRuntime.InterpolatedMoveInputVector.GetSafeNormal();
	const float CurrentAngle = FMath::RadiansToDegrees(FMath::Atan2(CurrentInput.X, CurrentInput.Y));
	const float TargetAngle = FMath::RadiansToDegrees(FMath::Atan2(TargetInput.X, TargetInput.Y));
	const float DeltaAngle = FMath::FindDeltaAngleDegrees(CurrentAngle, TargetAngle);
	const float MaxStep = FMath::Max(0.f, LocomotionSettings.MoveInputDirectionRotationRate) * DeltaTime;
	const float NextAngle = CurrentAngle + FMath::Clamp(DeltaAngle, -MaxStep, MaxStep);
	const float NextAngleRadians = FMath::DegreesToRadians(NextAngle);

	MovementRuntime.InterpolatedMoveInputVector = FVector2D(FMath::Sin(NextAngleRadians), FMath::Cos(NextAngleRadians));
	MovementRuntime.bHasInterpolatedMoveInput = true;
}

// 입력 보간 메모리를 지정 입력으로 즉시 맞춘다.
void ABAPlayerCharacter::SnapInterpolatedMoveInputTo(const FVector2D& MoveInput)
{
	const FVector2D SafeInput = MoveInput.GetSafeNormal();
	MovementRuntime.InterpolatedMoveInputVector = SafeInput;
	MovementRuntime.bHasInterpolatedMoveInput = !SafeInput.IsNearlyZero();
	MovementRuntime.InterpolatedMoveInputMemoryRemainingTime = MovementRuntime.bHasInterpolatedMoveInput
		? LocomotionSettings.MoveInputDirectionMemoryTime
		: 0.f;
}

// 보간 입력이 살아 있으면 보간 입력을, 아니면 원본 입력을 사용한다.
FVector2D ABAPlayerCharacter::GetInterpolatedMoveInputVector() const
{
	if (!IsMovementPhaseUsingRootMotion() && MovementRuntime.bHasInterpolatedMoveInput)
	{
		return MovementRuntime.InterpolatedMoveInputVector;
	}

	return MovementRuntime.MoveInputVector;
}

// 전달받은 2D 입력을 컨트롤 yaw 기준 월드 방향으로 변환한다.
FVector ABAPlayerCharacter::ConvertMoveInputToWorldDirection(const FVector2D& MoveInput) const
{
	if (MoveInput.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector2D SafeInput = MoveInput.SizeSquared() > 1.f ? MoveInput.GetSafeNormal() : MoveInput;
	const FRotator ControlRot = GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	return (Forward * SafeInput.Y + Right * SafeInput.X).GetSafeNormal();
}

// 월드 방향을 현재 컨트롤 yaw 기준 2D 입력으로 되돌린다.
FVector2D ABAPlayerCharacter::ConvertWorldDirectionToMoveInput(const FVector& WorldDirection) const
{
	FVector FlatDirection = WorldDirection;
	FlatDirection.Z = 0.f;
	if (FlatDirection.IsNearlyZero() || FlatDirection.Size2D() < LocomotionSettings.MoveInputDirectionVelocitySeedMinSpeed)
	{
		return FVector2D::ZeroVector;
	}

	FlatDirection.Normalize();

	const FRotator ControlRot = GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	return FVector2D(FVector::DotProduct(FlatDirection, Right), FVector::DotProduct(FlatDirection, Forward)).GetSafeNormal();
}
