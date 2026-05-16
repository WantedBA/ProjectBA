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
enum class EPlayerMovementPhase : uint8
{
	None,
	Start,
	Loop,
	Stop,
	Turn
};

UENUM(BlueprintType)
enum class EPlayerCombatMode : uint8
{
	None,
	Combat,
	Block
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

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float MoveInputDirectionRotationRate = 720.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float MoveInputDirectionMemoryTime = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion")
	float MoveInputDirectionVelocitySeedMinSpeed = 50.f;
};

USTRUCT(BlueprintType)
struct FBAPlayerMovementPhaseSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Movement|Phase")
	bool bUseStart = false;

	UPROPERTY(EditAnywhere, Category = "Movement|Phase")
	bool bUseStop = false;

	UPROPERTY(EditAnywhere, Category = "Movement|Phase")
	bool bUseTurn = false;

	UPROPERTY(EditAnywhere, Category = "Movement|Phase")
	bool bStartUsesRootMotion = true;

	UPROPERTY(EditAnywhere, Category = "Movement|Phase")
	bool bStopUsesRootMotion = true;

	UPROPERTY(EditAnywhere, Category = "Movement|Phase")
	bool bTurnUsesRootMotion = true;

	UPROPERTY(EditAnywhere, Category = "Movement|Phase")
	float TurnMinAngle = 90.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Phase")
	float TurnMinSpeed = 150.f;
};

USTRUCT(BlueprintType)
struct FBAPlayerMovementGaitSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Movement|Gait")
	FBAPlayerMovementPhaseSettings Walk;

	UPROPERTY(EditAnywhere, Category = "Movement|Gait")
	FBAPlayerMovementPhaseSettings Run;

	UPROPERTY(EditAnywhere, Category = "Movement|Gait")
	FBAPlayerMovementPhaseSettings Sprint;
};

USTRUCT(BlueprintType)
struct FBAPlayerSprintCostSettings
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	float StaminaCost = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	EPlayerStaminaCostType StaminaCostType = EPlayerStaminaCostType::Instant;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	float MinRequiredStamina = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	float RestartStaminaPercent = 70.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	bool bHasActionData = false;
};

struct FBAPlayerMovementRuntimeState
{
	EMovementState DesiredGait = EMovementState::Run;
	EMovementState ActiveGait = EMovementState::Run;
	EPlayerLocomotionMode LocomotionMode = EPlayerLocomotionMode::Free;
	EPlayerMovementPhase Phase = EPlayerMovementPhase::None;
	EPlayerCombatMode CombatMode = EPlayerCombatMode::None;
	FVector2D MoveInputVector = FVector2D::ZeroVector;
	FVector2D InterpolatedMoveInputVector = FVector2D::ZeroVector;
	FVector2D PhaseEntryInputVector = FVector2D::ZeroVector;
	FVector PhaseEntryWorldDirection = FVector::ZeroVector;
	bool bHasMoveInput = false;
	bool bHasInterpolatedMoveInput = false;
	bool bWaitingForPhaseAnimation = false;
	float InterpolatedMoveInputMemoryRemainingTime = 0.f;
	float PhaseEntryLocalAngle = 0.f;
	float PhaseElapsedTime = 0.f;
};

struct FBAPlayerSprintRuntimeState
{
	bool bLockedAfterExhausted = false;
};
