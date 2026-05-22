#include "Player/BAPlayerCharacter.h"

#include "Component/ActionComponent.h"
#include "Tables/ActionRows.h"
#include "Tables/BATableManager.h"

namespace
{
	EActionDirection GetActionDirectionFromRelativeInput(const FVector2D& RelativeInput)
	{
		constexpr float DirectionThreshold = 0.35f;
		if (RelativeInput.IsNearlyZero())
		{
			return EActionDirection::Any;
		}

		const FVector2D SafeInput = RelativeInput.SizeSquared() > 1.f ? RelativeInput.GetSafeNormal() : RelativeInput;
		const bool bForward = SafeInput.Y > DirectionThreshold;
		const bool bBackward = SafeInput.Y < -DirectionThreshold;
		const bool bRight = SafeInput.X > DirectionThreshold;
		const bool bLeft = SafeInput.X < -DirectionThreshold;

		if (bForward && bRight)
		{
			return EActionDirection::ForwardRight;
		}
		if (bForward && bLeft)
		{
			return EActionDirection::ForwardLeft;
		}
		if (bBackward && bRight)
		{
			return EActionDirection::BackwardRight;
		}
		if (bBackward && bLeft)
		{
			return EActionDirection::BackwardLeft;
		}
		if (bRight)
		{
			return EActionDirection::Right;
		}
		if (bLeft)
		{
			return EActionDirection::Left;
		}
		if (bBackward)
		{
			return EActionDirection::Backward;
		}

		return EActionDirection::Forward;
	}
}

// Phase가 루트모션 소유가 아닐 때만 저장된 이동 입력을 실제 이동에 사용한다.
void ABAPlayerCharacter::ApplyBufferedMoveInput()
{
	if (!MovementRuntime.bHasMoveInput || IsMovementPhaseUsingRootMotion() || IsActionMovementLocked())
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

	if (MovementRuntime.bHasMoveInput)
	{
		MovementRuntime.bSuppressVelocityFacingUntilMoveInput = false;
	}

	if (!MovementRuntime.bHasMoveInput)
	{
		if (MovementRuntime.bSuppressVelocityFacingUntilMoveInput)
		{
			SnapInterpolatedMoveInputTo(FVector2D::ZeroVector);
			return;
		}

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

EActionDirection ABAPlayerCharacter::GetActionDirectionFromMoveInput(const FVector2D& MoveInput) const
{
	if (MoveInput.IsNearlyZero())
	{
		return EActionDirection::Any;
	}

	return GetActionDirectionFromRelativeInput(MoveInput);
}

void ABAPlayerCharacter::FaceMoveInputDirection()
{
	const FVector MoveDirection = ConvertMoveInputToWorldDirection(GetMoveInputVector());
	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FRotator(CurrentRotation.Pitch, MoveDirection.Rotation().Yaw, CurrentRotation.Roll));
}

EActionDirection ABAPlayerCharacter::ResolveBufferedActionDirection(
	const int32 ActionTid,
	const EActionDirection BufferedDirection) const
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
	const FActionDataRow* ActionData = TableManager ? TableManager->FindActionData(ActionTid) : nullptr;
	if (!ActionData || ActionData->ActionType != EActionType::DodgeRoll)
	{
		return BufferedDirection;
	}

	return GetActionDirectionFromMoveInput(GetMoveInputVector());
}

bool ABAPlayerCharacter::IsActionMovementLocked() const
{
	// 상태 관리 중복됨 - 추후 통합 필요
	if (CanMoveWhileGuarding())
	{
		// 가드 액션 중 이동 의도가 있으면 BAPlayerState가 Guarding이어도 이동 입력을 소비한다.
		// 방어 판정은 여전히 GuardWindow에서만 열리고, 여기서는 하체 locomotion만 허용한다.
		// 단, 실제 속도 제한은 GetMovementAllowedGait()에서 Walk로 강제한다.
		return false;
	}

	return BAPlayerState != EBAPlayerState::None 
	|| !IsAlive()
	|| IsDamageReacting()
	|| (ActionComponent && ActionComponent->IsMovementLockedByAction());
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
