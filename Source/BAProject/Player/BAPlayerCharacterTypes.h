#pragma once

#include "CoreMinimal.h"
#include "Tables/PlayerEnums.h"
#include "BAPlayerCharacterTypes.generated.h"

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Walk,
	Run,
	Sprint
};

UENUM(BlueprintType)
enum class EPlayerLocomotionMode : uint8
{
	Free,
	Strafe
};

UENUM(BlueprintType)
enum class EPlayerCombatMode : uint8
{
	None,
	Combat,
	Block
};

enum class EPlayerTurnaroundState : uint8
{
	None,
	QueuedAfterSprintStop,
	ReadyToBeginAfterSprintStop,
	Playing
};

USTRUCT(BlueprintType)
struct FBAPlayerMovementSpeedSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float RunSpeed = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float SprintSpeed = 700.0f;
};

USTRUCT(BlueprintType)
struct FBAPlayerLocomotionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float FreeRotationRateYaw = 1440.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float FreeMaxAcceleration = 8192.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float FreeBrakingDecelerationWalking = 8192.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float FreeGroundFriction = 12.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float StrafeRotationRateYaw = 720.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float StrafeMaxAcceleration = 2048.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float StrafeBrakingDecelerationWalking = 2048.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float StrafeGroundFriction = 8.f;
};

USTRUCT(BlueprintType)
struct FBAPlayerSprintSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Movement|Sprint")
	float StrafeEntryBlendTime = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Movement|Sprint")
	float StrafeEntryOrientationSpeedRatio = 0.85f;

	UPROPERTY(EditAnywhere, Category = "Movement|Sprint")
	float StopRequestHoldTime = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Movement|Sprint")
	float StopMinSpeed = 150.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Sprint")
	float StopRequestWindowTime = 0.2f;
};

USTRUCT(BlueprintType)
struct FBAPlayerTurnaroundSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Movement|Turnaround")
	float MaxDuration = 3.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Turnaround")
	float PlayRateReferenceAngle = 90.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Turnaround")
	float PlayRateMinAngle = 30.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Turnaround")
	float MinPlayRate = 1.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Turnaround")
	float MaxPlayRate = 2.5f;
};

struct FBAPlayerMovementRuntimeState
{
	EMovementState MovementState = EMovementState::Run;
	EPlayerLocomotionMode LocomotionMode = EPlayerLocomotionMode::Free;
	EPlayerCombatMode CombatMode = EPlayerCombatMode::None;
	FVector2D MoveInputVector = FVector2D::ZeroVector;
	bool bHasMoveInput = false;
};

struct FBAPlayerSprintRuntimeState
{
	float StaminaCost = 0.f;
	EPlayerStaminaCostType StaminaCostType = EPlayerStaminaCostType::Instant;
	float MinRequiredStamina = 0.f;
	float RestartStaminaPercent = 0.f;
	bool bHasActionData = false;
	bool bLockedAfterExhausted = false;
	bool bKeepStrafeRotationDuringSprintEntry = false;
	bool bSprintStopRequested = false;
	bool bMovementLockedBySprintStop = false;
	bool bShouldTurnaroundAfterSprintStop = false;
	bool bCanRequestStopFromRecentExit = false;
	float EntryElapsedTime = 0.f;
	float StopRequestRemainingTime = 0.f;
	float StopRequestWindowRemainingTime = 0.f;
};

struct FBAPlayerTurnaroundRuntimeState
{
	EPlayerTurnaroundState State = EPlayerTurnaroundState::None;
	float ElapsedTime = 0.f;
	float AnimationAngle = 0.f;
};
