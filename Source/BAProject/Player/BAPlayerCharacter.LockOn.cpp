#include "Player/BAPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "LockOnTargetComponent.h"
#include "LockOnTargetExtensions/ControllerRotationExtension.h"
#include "TargetComponent.h"

bool ABAPlayerCharacter::IsLockOnTargetLocked() const
{
	const ULockOnTargetComponent* LockOnTargetComponent = FindComponentByClass<ULockOnTargetComponent>();
	return LockOnTargetComponent && LockOnTargetComponent->IsTargetLocked();
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
