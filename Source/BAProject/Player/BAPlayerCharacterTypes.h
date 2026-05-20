#pragma once

#include "CoreMinimal.h"
#include "Tables/ActionEnums.h"
#include "BAPlayerCharacterTypes.generated.h"

class AMapLadder;

// 플레이어가 요청하거나 실제로 사용 중인 이동 속도 단계.
UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Walk,
	Run,
	Sprint
};

// 캐릭터 회전 방식과 입력 해석 방식을 결정하는 이동 모드.
UENUM(BlueprintType)
enum class EPlayerLocomotionMode : uint8
{
	Free,
	Strafe
};

// 이동 애니메이션 상태 머신에서 사용하는 시작/루프/종료/회전 페이즈.
UENUM(BlueprintType)
enum class EPlayerMovementPhase : uint8
{
	None,
	Start,
	Loop,
	Stop,
	Turn
};

// 플레이어 애니메이션에 전달되는 전투 태세.
UENUM(BlueprintType)
enum class EPlayerCombatMode : uint8
{
	None,
	Combat,
	Block
};

// 플레이어 피격/가드 반응 애니메이션이 점유 중인 피동 상태.
UENUM(BlueprintType)
enum class EPlayerDamageReactionState : uint8
{
	None,
	HitReact,
	LargeHitReact,
	KnockDown,
	GuardHit,
	GuardBreak
};

// UserDataSubsystem 또는 디자이너 설정으로부터 채워지는 기본 이동 속도.
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

// Free/Strafe 모드별 CharacterMovement 파라미터와 입력 방향 보간 설정.
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

// 특정 gait에서 Start/Stop/Turn 페이즈를 사용할지와 루트 모션 여부를 정의한다.
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

// Walk/Run/Sprint 각 gait의 이동 페이즈 설정 묶음.
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

// ActionData 테이블에서 읽어 온 질주 스태미너 비용 및 재시작 조건.
USTRUCT(BlueprintType)
struct FBAPlayerSprintCostSettings
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	float StaminaCost = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	EActionStaminaCostType StaminaCostType = EActionStaminaCostType::Instant;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	float MinRequiredStamina = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	float RestartStaminaPercent = 70.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Sprint")
	bool bHasActionData = false;
};

// 사다리 이동 속도와 이탈 위치 보정값.
USTRUCT(BlueprintType)
struct FBAPlayerLadderSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Interaction|Ladder", meta = (ClampMin = "0.0"))
	float ClimbSpeedSlow = 120.f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Ladder", meta = (ClampMin = "0.0"))
	float ClimbSpeedFast = 280.f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Ladder", meta = (ClampMin = "0.0"))
	float SlideDownSpeed = 600.f; // 빠른 하강

	UPROPERTY(EditAnywhere, Category = "Interaction|Ladder", meta = (ClampMin = "0.0"))
	float ExitClearance = 80.f; // 이탈 시 사다리 너머로 밀어낼 거리
};

// 플레이어 이동 상태 머신이 프레임 사이에 유지하는 런타임 값.
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

// 질주 고갈 잠금과 스태미너 회복 일시정지 상태.
struct FBAPlayerSprintRuntimeState
{
	bool bLockedAfterExhausted = false;
	bool bStaminaRecoveryPaused = false;
};

// 현재 사다리 상호작용 상태.
struct FBAPlayerLadderRuntimeState
{
	bool bIsOnLadder = false;
	TWeakObjectPtr<AMapLadder> CurrentLadder;
};
