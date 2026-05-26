#include "Player/BAPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "LockOnTargetComponent.h"
#include "LockOnTargetExtensions/ControllerRotationExtension.h"
#include "Player/BADamageCameraShake.h"
#include "Player/BALandingCameraShake.h"

void ABAPlayerCharacter::InitializeCameraDefaults()
{
	ApplyCameraCollisionSettings();
	DamageReactionCameraShakeClass = UBADamageCameraShake::StaticClass();
	LandingRecoveryCameraShakeClass = UBALandingRecoveryCameraShake::StaticClass();
	DeathCameraShakeClass = UBADamageCameraShake::StaticClass();
}

void ABAPlayerCharacter::ApplyCameraCollisionSettings() const
{
	if (!SpringArm)
	{
		return;
	}

	SpringArm->bDoCollisionTest = bEnableCameraCollision;
	SpringArm->ProbeSize = CameraProbeSize;
	SpringArm->ProbeChannel = CameraProbeChannel;
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

void ABAPlayerCharacter::PlayConfiguredCameraShake(
	TSubclassOf<UCameraShakeBase> ShakeClass,
	const float Scale) const
{
	if (!ShakeClass || Scale <= 0.f)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->ClientStartCameraShake(ShakeClass, Scale);
	}
}

float ABAPlayerCharacter::ResolveDamageReactionCameraShakeScale(
	const EBADamageReactionType DamageReactionType,
	const bool bGuarding,
	const bool bGuardBreak) const
{
	if (bGuardBreak)
	{
		return DamageReactionCameraShakeScale * GuardBreakCameraShakeScale;
	}

	if (bGuarding)
	{
		return DamageReactionCameraShakeScale * GuardHitCameraShakeScale;
	}

	switch (DamageReactionType)
	{
	case EBADamageReactionType::KnockDown:
		return DamageReactionCameraShakeScale * KnockDownCameraShakeScale;
	case EBADamageReactionType::LargeHitReact:
		return DamageReactionCameraShakeScale * LargeHitReactCameraShakeScale;
	case EBADamageReactionType::HitReact:
	default:
		return DamageReactionCameraShakeScale * HitReactCameraShakeScale;
	}
}

void ABAPlayerCharacter::PlayDamageReactionCameraShake(
	const EBADamageReactionType DamageReactionType,
	const bool bGuarding,
	const bool bGuardBreak)
{
	const float ShakeScale = ResolveDamageReactionCameraShakeScale(DamageReactionType, bGuarding, bGuardBreak);
	PlayConfiguredCameraShake(DamageReactionCameraShakeClass, ShakeScale);
}

void ABAPlayerCharacter::PlayPerfectGuardCameraShake()
{
	PlayConfiguredCameraShake(
		DamageReactionCameraShakeClass,
		DamageReactionCameraShakeScale * PerfectGuardCameraShakeScale);
}

void ABAPlayerCharacter::PlayLandingRecoveryCameraShake()
{
	TSubclassOf<UCameraShakeBase> ShakeClass = LandingRecoveryCameraShakeClass;
	if (!ShakeClass)
	{
		ShakeClass = UBALandingRecoveryCameraShake::StaticClass();
	}

	PlayConfiguredCameraShake(ShakeClass, LandingRecoveryCameraShakeScale);
}

void ABAPlayerCharacter::PlayDeathCameraShake()
{
	TSubclassOf<UCameraShakeBase> ShakeClass = DeathCameraShakeClass;
	if (!ShakeClass)
	{
		ShakeClass = DamageReactionCameraShakeClass;
	}

	PlayConfiguredCameraShake(ShakeClass, DeathCameraShakeScale);
}
