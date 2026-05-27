#include "Player/BAPlayerCharacter.h"

#include "Component/StatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/MapLadder.h"

bool ABAPlayerCharacter::IsOnLadder() const
{
	return LadderRuntime.bIsOnLadder;
}

AMapLadder* ABAPlayerCharacter::GetCurrentLadder() const
{
	return LadderRuntime.CurrentLadder.Get();
}

void ABAPlayerCharacter::EnterLadder(AMapLadder* Ladder, const FVector& EntryLocation, const FRotator& FaceRotation)
{
	if (!Ladder)
	{
		return;
	}

	LadderRuntime.CurrentLadder = Ladder;
	LadderRuntime.bIsOnLadder = true;
	ResetMovementRuntimeForLadder();
	SetActiveGaitAndSpeed(GetMovementAllowedGait(MovementRuntime.DesiredGait));

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Flying);
		MovementComponent->bOrientRotationToMovement = false;
	}

	bUseControllerRotationYaw = false;
	SetActorLocationAndRotation(EntryLocation, FaceRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void ABAPlayerCharacter::ExitLadder(const FVector& ExitLocation)
{
	LadderRuntime.bIsOnLadder = false;
	LadderRuntime.CurrentLadder.Reset();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	SetActorLocation(ExitLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActiveGaitAndSpeed(GetMovementAllowedGait(MovementRuntime.DesiredGait));
	SyncFreeStrafeFacingMode();
}

float ABAPlayerCharacter::GetLadderClimbVelocity() const
{
	if (!IsOnLadder())
	{
		return 0.f;
	}

	return CalculateLadderClimbSpeed(MovementRuntime.MoveInputVector.Y);
}

void ABAPlayerCharacter::TickLadderClimb(const float DeltaTime)
{
	UnlockSprintAfterRecovery();

	if (!LadderRuntime.CurrentLadder.IsValid())
	{
		ExitLadder(GetActorLocation());
		return;
	}

	const float VerticalInput = MovementRuntime.MoveInputVector.Y; // W/S
	const bool bHasVerticalInput = FMath::Abs(VerticalInput) > KINDA_SMALL_NUMBER;
	const bool bSprintActive = bHasVerticalInput && IsLadderSprintRequested();
	const EMovementState LadderGait = bSprintActive
		? EMovementState::Sprint
		: GetMovementAllowedGait(MovementRuntime.DesiredGait);

	if (MovementRuntime.ActiveGait != LadderGait)
	{
		SetActiveGaitAndSpeed(LadderGait);
	}

	const float ClimbSpeed = CalculateLadderClimbSpeed(VerticalInput);
	if (bSprintActive)
	{
		// Sprint 시 스태미너 소비 (위/아래 모두). 고갈되면 Run으로 강등
		DrainLadderSprintStamina(DeltaTime);
	}

	// XY를 사다리 진입 라인(BottomEntry XY)에 강제 락 — 측면 진입 시 루트모션/외부 힘으로 인한 횡 드리프트 차단
	const AMapLadder* Ladder = LadderRuntime.CurrentLadder.Get();
	const FVector BottomLocation = Ladder->GetBottomEntryLocation();
	const FVector CurrentLocation = GetActorLocation();
	const float NewZ = CurrentLocation.Z + ClimbSpeed * DeltaTime;
	SetActorLocation(
		FVector(BottomLocation.X, BottomLocation.Y, NewZ),
		false, nullptr, ETeleportType::TeleportPhysics);

	// 자동 이탈 - 이동 방향과 일치할 때만 (진입 직후 즉시 트리거 방지)
	const FVector ActorLocation = GetActorLocation();
	const FVector TopLocation = Ladder->GetTopEntryLocation();

	if (ClimbSpeed > 0.f && ActorLocation.Z >= TopLocation.Z)
	{
		// 위로 올라가다가 상단 도달 - 사다리 너머(forward) 약간 밀어내고 이탈
		ExitLadder(TopLocation + GetActorForwardVector() * LadderSettings.ExitClearance);
	}
	else if (ClimbSpeed < 0.f && ActorLocation.Z <= BottomLocation.Z)
	{
		// 아래로 내려가다가 하단 도달 - BottomEntry 위치로
		ExitLadder(BottomLocation);
	}
}

float ABAPlayerCharacter::CalculateLadderClimbSpeed(const float VerticalInput) const
{
	const bool bSprintRequested = IsLadderSprintRequested();
	if (VerticalInput > KINDA_SMALL_NUMBER)
	{
		// 위로: Sprint면 빠르게
		return bSprintRequested ? LadderSettings.ClimbSpeedFast : LadderSettings.ClimbSpeedSlow;
	}

	if (VerticalInput < -KINDA_SMALL_NUMBER)
	{
		// 아래로: Sprint면 슬라이드 다운
		return bSprintRequested ? -LadderSettings.SlideDownSpeed : -LadderSettings.ClimbSpeedSlow;
	}

	return 0.f;
}

bool ABAPlayerCharacter::IsLadderSprintRequested() const
{
	return MovementRuntime.DesiredGait == EMovementState::Sprint && IsSprintAllowedByStamina();
}

void ABAPlayerCharacter::DrainLadderSprintStamina(const float DeltaTime)
{
	if (!StatComponent || SprintCostSettings.StaminaCost <= 0.f)
	{
		return;
	}

	StatComponent->ConsumeStamina(CalculateSprintStaminaDrain(DeltaTime));
	LockSprintUntilRecovered();

	if (!IsSprintAllowedByStamina())
	{
		SetMovementState(EMovementState::Run);
		SetActiveGaitAndSpeed(EMovementState::Run);
	}
}

void ABAPlayerCharacter::ResetMovementRuntimeForLadder()
{
	MovementRuntime.Phase = EPlayerMovementPhase::None;
	MovementRuntime.PhaseElapsedTime = 0.f;
	MovementRuntime.PhaseEntryInputVector = FVector2D::ZeroVector;
	MovementRuntime.PhaseEntryWorldDirection = FVector::ZeroVector;
	MovementRuntime.PhaseEntryLocalAngle = 0.f;
	MovementRuntime.bWaitingForPhaseAnimation = false;
	SnapInterpolatedMoveInputTo(MovementRuntime.MoveInputVector);
}
