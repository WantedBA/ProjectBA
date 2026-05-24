#include "Player/BAPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "LockOnTargetComponent.h"
#include "LockOnTargetExtensions/ControllerRotationExtension.h"
#include "TargetComponent.h"

/*
 * LockOn Policy Summary
 *
 * 입력 결과
 * - LockOnAction 입력은 ABAPlayerController::OnLockOnStarted()로 들어온다.
 * - 컨트롤러는 캐릭터의 ToggleLockOnTargeting()만 호출한다.
 * - ToggleLockOnTargeting()은 ULockOnTargetComponent::EnableTargeting()을 호출한다.
 * - EnableTargeting()은 타겟이 없으면 탐색하고, 이미 락온 중이면 해제한다.
 * - 락온 중 LookAction 입력은 SwitchLockOnTargetInput()으로 들어와 SwitchTargetYaw/Pitch에 전달된다.
 * - 플러그인은 입력 버퍼가 임계값을 넘으면 WeightedTargetHandler로 입력 방향의 후보를 다시 고른다.
 *
 * 락온 성공 결과
 * - OnTargetLocked 델리게이트가 HandleLockOnTargetLocked()를 호출한다.
 * - bForceStrafeWhileLockedOn이 true면 현재 locomotion을 저장하고 Strafe로 고정한다.
 * - Sprint 입력은 Free와 같은 규칙으로 허용한다.
 * - Run/Sprint 전환 시 MaxWalkSpeed는 SpeedUpInterpRate/SlowDownInterpRate로 보간된다.
 * - Strafe Run에서 Sprint로 넘어갈 때 캐릭터 yaw는 StrafeSprintFacingRotationRateYaw로 보간된다.
 *
 * 락온 해제 결과
 * - OnTargetUnlocked 델리게이트가 HandleLockOnTargetUnlocked()를 호출한다.
 * - 락온 때문에 Strafe로 바뀐 경우에만 이전 locomotion으로 복구한다.
 *
 * 카메라 결과
 * - ControllerRotationExtension이 컨트롤러 회전을 타겟 방향으로 돌린다.
 * - ConfigureLockOnCameraDefaults()는 카메라 상대 Pitch를 기준으로 PitchOffset을 보정한다.
 * - 화면 중앙 높이를 조정할 때는 LockOnAdditionalControllerPitchOffset 값을 바꾼다.
 *
 * 타겟 사망 결과
 * - EnemyBase가 사망 시 일정 지연 후 TargetComponent::SetCanBeCaptured(false)를 호출한다.
 * - WeightedTargetHandler는 StateInvalidation을 받고 다음 타겟을 찾거나 락온을 해제한다.
 */

bool ABAPlayerCharacter::IsLockOnTargetLocked() const
{
	const ULockOnTargetComponent* LockOnTargetComponent = FindComponentByClass<ULockOnTargetComponent>();
	return LockOnTargetComponent && LockOnTargetComponent->IsTargetLocked();
}

void ABAPlayerCharacter::ToggleLockOnTargeting()
{
	if (ULockOnTargetComponent* LockOnTargetComponent = FindComponentByClass<ULockOnTargetComponent>())
	{
		LockOnTargetComponent->EnableTargeting();
	}
}

void ABAPlayerCharacter::SwitchLockOnTargetInput(const FVector2D& SwitchInput)
{
	ULockOnTargetComponent* LockOnTargetComponent = FindComponentByClass<ULockOnTargetComponent>();
	if (!LockOnTargetComponent || !LockOnTargetComponent->IsTargetLocked())
	{
		return;
	}

	LockOnTargetComponent->SwitchTargetYaw(SwitchInput.X);
	LockOnTargetComponent->SwitchTargetPitch(SwitchInput.Y);
}

void ABAPlayerCharacter::BindLockOnTargetCallbacks()
{
	ULockOnTargetComponent* LockOnTargetComponent = FindComponentByClass<ULockOnTargetComponent>();
	if (!LockOnTargetComponent)
	{
		return;
	}

	LockOnTargetComponent->OnTargetLocked.AddDynamic(this, &ABAPlayerCharacter::HandleLockOnTargetLocked);
	LockOnTargetComponent->OnTargetUnlocked.AddDynamic(this, &ABAPlayerCharacter::HandleLockOnTargetUnlocked);
}

void ABAPlayerCharacter::ConfigureLockOnCameraDefaults()
{
	if (!bAutoCalibrateLockOnControllerPitch)
	{
		return;
	}

	ULockOnTargetComponent* LockOnTargetComponent = FindComponentByClass<ULockOnTargetComponent>();
	UControllerRotationExtension* RotationExtension = LockOnTargetComponent
		? Cast<UControllerRotationExtension>(LockOnTargetComponent->FindExtensionByClass(UControllerRotationExtension::StaticClass()))
		: nullptr;

	if (!RotationExtension)
	{
		return;
	}

	const float CameraRelativePitch = Camera ? Camera->GetRelativeRotation().Pitch : 0.f;
	RotationExtension->PitchOffset = -CameraRelativePitch + LockOnAdditionalControllerPitchOffset;
}

void ABAPlayerCharacter::HandleLockOnTargetLocked(UTargetComponent* /*Target*/, FName /*Socket*/)
{
	EnterLockOnStrafeMode();
}

void ABAPlayerCharacter::HandleLockOnTargetUnlocked(UTargetComponent* /*UnlockedTarget*/, FName /*Socket*/)
{
	RestoreLocomotionModeAfterLockOn();
}

void ABAPlayerCharacter::EnterLockOnStrafeMode()
{
	if (!bForceStrafeWhileLockedOn)
	{
		return;
	}

	if (!bLockOnForcedStrafeActive)
	{
		LocomotionModeBeforeLockOn = MovementRuntime.LocomotionMode;
		bLockOnForcedStrafeActive = true;
	}

	SetLocomotionMode(EPlayerLocomotionMode::Strafe);
}

void ABAPlayerCharacter::RestoreLocomotionModeAfterLockOn()
{
	if (!bLockOnForcedStrafeActive)
	{
		return;
	}

	bLockOnForcedStrafeActive = false;
	SetLocomotionMode(LocomotionModeBeforeLockOn);
}
