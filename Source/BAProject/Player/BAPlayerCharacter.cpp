#include "Player/BAPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Component/StatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Instance/UserDataSubsystem.h"
#include "Component/InteractorComponent.h"
#include "Materials/MaterialInterface.h"
#include "World/MapLadder.h"

namespace
{
	// TODO: id 하드코딩
	constexpr int32 SprintActionTid = 10020;
	constexpr uint64 SprintDebugMessageKey = 12020;
	constexpr uint64 SprintStopDebugMessageKey = 12021;
	constexpr float DefaultSprintRestartStaminaPercent = 70.f;
}

ABAPlayerCharacter::ABAPlayerCharacter()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMesh(TEXT("/Game/Character/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (CharacterMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(CharacterMesh.Object);
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> CharacterAnim(TEXT("/Game/Character/Player/Animation/ABP_Player.ABP_Player_C"));
	if (CharacterAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(CharacterAnim.Class);
	}
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

	// 스탯 컴포넌트 생성
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));

	// 상호작용 컴포넌트 생성
	InteractorComponent = CreateDefaultSubobject<UInteractorComponent>(TEXT("InteractorComponent"));

	// C++ 동적 생성이라 BP 슬롯이 없으므로 외곽선용 PostProcess 머티리얼을 코드에서 주입
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OutlinePPMat(
		TEXT("/Game/UI/Interaction/M_PP_Outline.M_PP_Outline"));
	if (OutlinePPMat.Succeeded() && InteractorComponent)
	{
		InteractorComponent->SetOutlineMaterial(OutlinePPMat.Object);
	}

	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.f, 0.f, -90.f),
		FRotator(0.f, -90.f, 0.f)
	);

	// back view, 3인칭 설정
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 330.f;
	SpringArm->SocketOffset = FVector(0.f, 0.f, 160.f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	
	// camera spring arm 충돌 활성화
	SpringArm->bDoCollisionTest = true; 

	// camera 설정
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->SetRelativeRotation(FRotator(-17.f, 0.f, 0.f));
	Camera->bUsePawnControlRotation = false;
	
	// 마우스 카메라 제어 Yaw축만 허용
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	ApplyLocomotionMovementPolicy();
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void ABAPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializeFromTable();
	ApplyLocomotionMovementPolicy();
	SetMovementState(EMovementState::Run);

	// StatComponent가 존재할 경우 변경된 델리게이트에 핸들러 함수 바인딩
	if (StatComponent)
	{
		// HP
		StatComponent->OnHPChanged.AddDynamic(this, &ABAPlayerCharacter::OnHealthChanged);

		// Stamina
		StatComponent->OnStaminaChanged.AddDynamic(this, &ABAPlayerCharacter::OnStaminaChanged);

		OnHealthChanged(StatComponent->GetCurrentHP(), StatComponent->GetMaxHP());
		OnStaminaChanged(StatComponent->GetCurrentStamina(), StatComponent->GetMaxStamina());
	}
}

void ABAPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsOnLadder)
	{
		TickLadderClimb(DeltaTime);
		return;
	}

	UpdateSprintStopRequest(DeltaTime);
	UpdateSprintStopRequestWindow(DeltaTime);
	UpdateTurnaroundRotation(DeltaTime);

	if (CurrentMovementState != EMovementState::Sprint)
	{
		UpdateSprintExhaustionLock();
		return;
	}

	UpdateSprintEntryRotation(DeltaTime);
	UpdateSprintExhaustionLock();
	LockSprintIfExhausted();

	if (!IsSprintMovementActive() || !CanSprint())
	{
		SetMovementState(EMovementState::Run);
		return;
	}

	ConsumeSprintStamina(DeltaTime);

	if (!CanSprint())
	{
		SetMovementState(EMovementState::Run);
	}
}

void ABAPlayerCharacter::Attack()
{
	
}

void ABAPlayerCharacter::InitializeFromTable()
{
	const UUserDataSubsystem* UserDataSubsystem = UUserDataSubsystem::Get(this);
	if (!UserDataSubsystem)
	{
		return;
	}

	const FPlayerBaseStat BaseStat = UserDataSubsystem->GetBaseStat();
	WalkSpeed = BaseStat.WalkSpeed;
	RunSpeed = BaseStat.RunSpeed;
	SprintSpeed = BaseStat.SprintSpeed;

	StatComponent->InitializeStats(
		BaseStat.MaxHp,	
		BaseStat.MaxStamina,
		BaseStat.StaminaRecoveryPerSecond,
		BaseStat.StaminaRecoveryDelay,
		WalkSpeed,
		RunSpeed,
		SprintSpeed,
		BaseStat.BaseAttack,
		BaseStat.BaseAttackSpeed,
		BaseStat.BaseDefence
	);

	if (const FPlayerActionData* SprintActionData = UserDataSubsystem->FindActionData(SprintActionTid))
	{
		SprintStaminaCost = FMath::Max(0.f, SprintActionData->StaminaCost);
		SprintStaminaCostType = SprintActionData->StaminaCostType;
		SprintMinRequiredStamina = FMath::Max(0.f, SprintActionData->MinRequiredStamina);
		SprintRestartStaminaPercent = SprintActionData->SprintRestartStaminaPercent > 0.f
			? FMath::Clamp(SprintActionData->SprintRestartStaminaPercent, 0.f, 100.f)
			: DefaultSprintRestartStaminaPercent;
		bHasSprintActionData = true;
	}
	else
	{
		SprintStaminaCost = 0.f;
		SprintStaminaCostType = EPlayerStaminaCostType::Instant;
		SprintMinRequiredStamina = 0.f;
		SprintRestartStaminaPercent = 0.f;
		bHasSprintActionData = false;
	}

	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void ABAPlayerCharacter::SetMovementState(EMovementState NewState)
{
	UpdateSprintExhaustionLock();

	if (NewState == EMovementState::Sprint)
	{
		LockSprintIfExhausted();
	}

	if (NewState == EMovementState::Sprint && !CanSprint())
	{
		NewState = EMovementState::Run;
	}

	const EMovementState PreviousMovementState = CurrentMovementState;
	if (CurrentMovementState == NewState)
	{
		return;
	}

	CurrentMovementState = NewState;
	if (CurrentMovementState == EMovementState::Sprint)
	{
		ClearSprintStopRequest();
		bSprintEntryRotationLocked = CurrentLocomotionMode == EPlayerLocomotionMode::Strafe;
		bSprintStopShouldTurnaround = CurrentLocomotionMode == EPlayerLocomotionMode::Strafe;
		SprintEntryElapsedTime = 0.f;
	}
	else if (PreviousMovementState == EMovementState::Sprint)
	{
		StartSprintStopRequestWindow();

		if (!bHasMoveInput && CanRequestSprintStop())
		{
			RequestSprintStop();
		}

		bSprintEntryRotationLocked = false;
		SprintEntryElapsedTime = 0.f;
	}
	
	float NewSpeed = RunSpeed; // 기본값
	switch (CurrentMovementState)
	{
	case EMovementState::Walk:
		NewSpeed = WalkSpeed;
		break;
	case EMovementState::Run:
		NewSpeed = RunSpeed;
		break;
	case EMovementState::Sprint:
		NewSpeed = SprintSpeed;
		break;
	}
	
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
	ApplyLocomotionMovementPolicy();
}

void ABAPlayerCharacter::SetMoveInputVector(const FVector2D& NewMoveInput)
{
	MoveInputVector = NewMoveInput;
	if (MoveInputVector.SizeSquared() > 1.f)
	{
		MoveInputVector.Normalize();
	}

	SetHasMoveInput(!MoveInputVector.IsNearlyZero());
}

void ABAPlayerCharacter::SetLocomotionMode(const EPlayerLocomotionMode NewMode)
{
	if (CurrentLocomotionMode == NewMode)
	{
		return;
	}

	CurrentLocomotionMode = NewMode;
	ApplyLocomotionMovementPolicy();
}

void ABAPlayerCharacter::SetCombatMode(const EPlayerCombatMode NewMode)
{
	CurrentCombatMode = NewMode;
}

EMovementState ABAPlayerCharacter::GetMovementState() const
{
	return CurrentMovementState;
}

EPlayerLocomotionMode ABAPlayerCharacter::GetLocomotionMode() const
{
	return CurrentLocomotionMode;
}

EPlayerCombatMode ABAPlayerCharacter::GetCombatMode() const
{
	return CurrentCombatMode;
}

bool ABAPlayerCharacter::HasMoveInput() const
{
	return bHasMoveInput;
}

FVector2D ABAPlayerCharacter::GetMoveInputVector() const
{
	return MoveInputVector;
}

FVector ABAPlayerCharacter::GetMoveInputWorldDirection() const
{
	if (!bHasMoveInput)
	{
		return FVector::ZeroVector;
	}

	const FRotator ControlRot = GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	return (Forward * MoveInputVector.Y + Right * MoveInputVector.X).GetSafeNormal();
}

FVector ABAPlayerCharacter::GetMoveInputLocalDirection() const
{
	const FVector WorldDirection = GetMoveInputWorldDirection();
	if (WorldDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	return GetActorTransform().InverseTransformVectorNoScale(WorldDirection).GetSafeNormal();
}

float ABAPlayerCharacter::GetMoveInputDirectionAngle() const
{
	const FVector LocalDirection = GetMoveInputLocalDirection();
	if (LocalDirection.IsNearlyZero())
	{
		return 0.f;
	}

	return FMath::RadiansToDegrees(FMath::Atan2(LocalDirection.Y, LocalDirection.X));
}

float ABAPlayerCharacter::GetVelocityDirectionAngle() const
{
	FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(GetVelocity());
	LocalVelocity.Z = 0.f;

	if (LocalVelocity.IsNearlyZero())
	{
		return 0.f;
	}

	LocalVelocity.Normalize();
	return FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
}

float ABAPlayerCharacter::GetGroundSpeed() const
{
	const FVector Velocity = GetVelocity();
	return FVector(Velocity.X, Velocity.Y, 0.f).Size();
}

bool ABAPlayerCharacter::IsSprintLockedAfterExhausted() const
{
	return bSprintLockedAfterExhausted;
}

bool ABAPlayerCharacter::IsSprintEntryRotationLocked() const
{
	return ShouldUseSprintEntryRotationLock();
}

float ABAPlayerCharacter::GetSprintTurnDeltaAngle() const
{
	return FMath::FindDeltaAngleDegrees(GetVelocityDirectionAngle(), GetMoveInputDirectionAngle());
}

float ABAPlayerCharacter::GetTurnaroundToControlRotationAngle() const
{
	if (!IsTurnaroundRequested())
	{
		return 0.f;
	}

	return TurnaroundAnimationAngle;
}

float ABAPlayerCharacter::GetTurnaroundPlayRate() const
{
	if (!IsTurnaroundRequested())
	{
		return 1.f;
	}

	const float AbsAngle = FMath::Abs(TurnaroundAnimationAngle);
	const float SafeAngle = FMath::Max(AbsAngle, TurnaroundPlayRateMinAngle);
	const float RawPlayRate = TurnaroundPlayRateReferenceAngle / SafeAngle;
	return FMath::Clamp(RawPlayRate, TurnaroundMinPlayRate, TurnaroundMaxPlayRate);
}

bool ABAPlayerCharacter::IsSprintStopRequested() const
{
	return bSprintStopRequested;
}

void ABAPlayerCharacter::ClearSprintStopRequest()
{
	const bool bWasSprintStopActive = bSprintStopRequested
		|| bSprintStopMovementLocked
		|| bTurnaroundQueuedAfterSprintStop
		|| bCanBeginTurnaroundAfterSprintStop
		|| bTurnaroundRequested;

	bSprintStopRequested = false;
	bSprintStopMovementLocked = false;
	bSprintStopStartedFromStrafe = false;
	bSprintStopShouldTurnaround = false;
	bTurnaroundQueuedAfterSprintStop = false;
	bCanBeginTurnaroundAfterSprintStop = false;
	bTurnaroundRequested = false;
	SprintStopRequestRemainingTime = 0.f;
	bCanRequestSprintStopFromRecentExit = false;
	SprintStopRequestWindowRemainingTime = 0.f;
	TurnaroundElapsedTime = 0.f;
	TurnaroundAnimationAngle = 0.f;

	if (bWasSprintStopActive)
	{
		ApplyLocomotionMovementPolicy();
	}
}

void ABAPlayerCharacter::CompleteSprintStopAnimation()
{
	bSprintStopRequested = false;
	SprintStopRequestRemainingTime = 0.f;
	bCanRequestSprintStopFromRecentExit = false;
	SprintStopRequestWindowRemainingTime = 0.f;

	if (bTurnaroundRequested)
	{
		return;
	}

	if (bTurnaroundQueuedAfterSprintStop)
	{
		TurnaroundAnimationAngle = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, GetControlRotation().Yaw);
		bCanBeginTurnaroundAfterSprintStop = true;
		ApplyLocomotionMovementPolicy();
		return;
	}

	bSprintStopMovementLocked = false;
	bSprintStopStartedFromStrafe = false;
	bSprintStopShouldTurnaround = false;
	ApplyLocomotionMovementPolicy();
}

bool ABAPlayerCharacter::IsTurnaroundRequested() const
{
	return bCanBeginTurnaroundAfterSprintStop || bTurnaroundRequested;
}

bool ABAPlayerCharacter::IsTurnaroundQueuedAfterSprintStop() const
{
	return bTurnaroundQueuedAfterSprintStop;
}

void ABAPlayerCharacter::BeginTurnaroundAnimation()
{
	if (bTurnaroundRequested || !bCanBeginTurnaroundAfterSprintStop)
	{
		return;
	}

	bTurnaroundQueuedAfterSprintStop = false;
	bCanBeginTurnaroundAfterSprintStop = false;
	bTurnaroundRequested = true;
	TurnaroundElapsedTime = 0.f;
	ApplyLocomotionMovementPolicy();
}

void ABAPlayerCharacter::CompleteTurnaroundAnimation()
{
	bTurnaroundQueuedAfterSprintStop = false;
	bCanBeginTurnaroundAfterSprintStop = false;
	bTurnaroundRequested = false;
	bSprintStopMovementLocked = false;
	bSprintStopStartedFromStrafe = false;
	bSprintStopShouldTurnaround = false;
	TurnaroundElapsedTime = 0.f;
	TurnaroundAnimationAngle = 0.f;
	ApplyLocomotionMovementPolicy();
}

void ABAPlayerCharacter::EnterLadder(AMapLadder* Ladder, const FVector& EntryLocation, const FRotator& FaceRotation)
{
	if (!Ladder) return;

	CurrentLadder = Ladder;
	bIsOnLadder = true;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Flying);
		Move->bOrientRotationToMovement = false;
	}

	SetActorLocationAndRotation(EntryLocation, FaceRotation);
}

void ABAPlayerCharacter::ExitLadder(const FVector& ExitLocation)
{
	bIsOnLadder = false;
	CurrentLadder.Reset();

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}

	SetActorLocation(ExitLocation);
	ApplyLocomotionMovementPolicy();
}

float ABAPlayerCharacter::GetLadderClimbVelocity() const
{
	if (!bIsOnLadder) return 0.f;
	const float VInput = MoveInputVector.Y;
	const bool bSprint = (CurrentMovementState == EMovementState::Sprint);
	if (VInput > KINDA_SMALL_NUMBER)
	{
		return bSprint ? LadderClimbSpeedFast : LadderClimbSpeedSlow;
	}
	if (VInput < -KINDA_SMALL_NUMBER)
	{
		return bSprint ? -LadderSlideDownSpeed : -LadderClimbSpeedSlow;
	}
	return 0.f;
}

bool ABAPlayerCharacter::CanSprint() const
{
	if (!bHasSprintActionData || !StatComponent)
	{
		return false;
	}

	const float CurrentStamina = StatComponent->GetCurrentStamina();
	const float SprintRestartStamina = StatComponent->GetMaxStamina() * SprintRestartStaminaPercent / 100.f;

	if (bSprintLockedAfterExhausted && CurrentStamina < SprintRestartStamina)
	{
		return false;
	}

	return CurrentStamina >= SprintMinRequiredStamina
		&& (SprintStaminaCost <= 0.f ||  CurrentStamina > 0.f);
}

bool ABAPlayerCharacter::IsSprintMovementActive() const
{
	return bHasMoveInput;
}

void ABAPlayerCharacter::ConsumeSprintStamina(const float DeltaTime)
{
	if (!StatComponent || SprintStaminaCost <= 0.f)
	{
		return;
	}

	const float CurrentStamina = StatComponent->GetCurrentStamina();
	const float ConsumeAmount = CalculateSprintStaminaCost(DeltaTime);
	StatComponent->SetCurrentStamina(CurrentStamina - ConsumeAmount);

	LockSprintIfExhausted();
}

float ABAPlayerCharacter::CalculateSprintStaminaCost(const float DeltaTime) const
{
	if (!StatComponent)
	{
		return 0.f;
	}

	switch (SprintStaminaCostType)
	{
	case EPlayerStaminaCostType::PerSecond:
		return StatComponent->GetMaxStamina() * SprintStaminaCost / 100.f * DeltaTime;
	case EPlayerStaminaCostType::Instant:
	default:
		return SprintStaminaCost;
	}
}

void ABAPlayerCharacter::LockSprintIfExhausted()
{
	if (!StatComponent || SprintStaminaCost <= 0.f)
	{
		return;
	}

	if (StatComponent->GetCurrentStamina() <= 0.f)
	{
		bSprintLockedAfterExhausted = true;
	}
}

void ABAPlayerCharacter::UpdateSprintExhaustionLock()
{
	if (!bSprintLockedAfterExhausted || !StatComponent)
	{
		return;
	}

	const float SprintRestartStamina = StatComponent->GetMaxStamina() * SprintRestartStaminaPercent / 100.f;
	if (StatComponent->GetCurrentStamina() >= SprintRestartStamina)
	{
		bSprintLockedAfterExhausted = false;
	}
}

void ABAPlayerCharacter::ApplyLocomotionMovementPolicy()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (bTurnaroundRequested)
	{
		bUseControllerRotationYaw = false;
		MovementComponent->bOrientRotationToMovement = false;
		MovementComponent->MaxAcceleration = StrafeMaxAcceleration;
		MovementComponent->BrakingDecelerationWalking = StrafeBrakingDecelerationWalking;
		MovementComponent->GroundFriction = StrafeGroundFriction;
		return;
	}

	if (ShouldUseSprintMovementPolicy() && !ShouldUseSprintEntryRotationLock())
	{
		bUseControllerRotationYaw = false;
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->RotationRate = FRotator(0.f, FreeRotationRateYaw, 0.f);
		MovementComponent->MaxAcceleration = FreeMaxAcceleration;
		MovementComponent->BrakingDecelerationWalking = FreeBrakingDecelerationWalking;
		MovementComponent->GroundFriction = FreeGroundFriction;
		return;
	}

	switch (CurrentLocomotionMode)
	{
	case EPlayerLocomotionMode::Strafe:
		bUseControllerRotationYaw = true;
		MovementComponent->bOrientRotationToMovement = false;
		MovementComponent->RotationRate = FRotator(0.f, StrafeRotationRateYaw, 0.f);
		MovementComponent->MaxAcceleration = StrafeMaxAcceleration;
		MovementComponent->BrakingDecelerationWalking = StrafeBrakingDecelerationWalking;
		MovementComponent->GroundFriction = StrafeGroundFriction;
		break;
	case EPlayerLocomotionMode::Free:
	default:
		bUseControllerRotationYaw = false;
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->RotationRate = FRotator(0.f, FreeRotationRateYaw, 0.f);
		MovementComponent->MaxAcceleration = FreeMaxAcceleration;
		MovementComponent->BrakingDecelerationWalking = FreeBrakingDecelerationWalking;
		MovementComponent->GroundFriction = FreeGroundFriction;
		break;
	}
}

void ABAPlayerCharacter::UpdateSprintEntryRotation(const float DeltaTime)
{
	if (!bSprintEntryRotationLocked)
	{
		return;
	}

	SprintEntryElapsedTime += DeltaTime;

	const float OrientationSpeed = SprintSpeed * SprintStrafeEntryOrientationSpeedRatio;
	const bool bHasBlendedLongEnough = SprintEntryElapsedTime >= SprintStrafeEntryBlendTime;
	const bool bReachedSprintOrientationSpeed = GetGroundSpeed() >= OrientationSpeed;

	if (bHasBlendedLongEnough && bReachedSprintOrientationSpeed)
	{
		bSprintEntryRotationLocked = false;
		ApplyLocomotionMovementPolicy();
	}
}

bool ABAPlayerCharacter::ShouldUseSprintEntryRotationLock() const
{
	return CurrentMovementState == EMovementState::Sprint && bSprintEntryRotationLocked;
}

void ABAPlayerCharacter::RequestSprintStop()
{
	bSprintStopRequested = true;
	bSprintStopMovementLocked = true;
	bSprintStopStartedFromStrafe = bSprintStopShouldTurnaround;
	bTurnaroundQueuedAfterSprintStop = bSprintStopShouldTurnaround;
	bCanBeginTurnaroundAfterSprintStop = false;
	bSprintStopShouldTurnaround = false;
	bTurnaroundRequested = false;
	bCanRequestSprintStopFromRecentExit = false;
	SprintStopRequestWindowRemainingTime = 0.f;
	SprintStopRequestRemainingTime = FMath::Max(0.f, SprintStopRequestHoldTime);
	ApplyLocomotionMovementPolicy();
}

void ABAPlayerCharacter::UpdateSprintStopRequest(const float DeltaTime)
{
	if (!bSprintStopRequested)
	{
		return;
	}

	SprintStopRequestRemainingTime -= DeltaTime;
	if (SprintStopRequestRemainingTime <= 0.f)
	{
		bSprintStopRequested = false;
		SprintStopRequestRemainingTime = 0.f;
	}
}

void ABAPlayerCharacter::SetHasMoveInput(const bool bNewHasMoveInput)
{
	bHasMoveInput = bNewHasMoveInput;
	if (bHasMoveInput && (bSprintStopRequested
		|| bSprintStopMovementLocked
		|| bTurnaroundQueuedAfterSprintStop
		|| IsTurnaroundRequested()))
	{
		ClearSprintStopRequest();
	}

	if (!bHasMoveInput)
	{
		MoveInputVector = FVector2D::ZeroVector;
	}

	if (!bHasMoveInput && CurrentMovementState == EMovementState::Sprint)
	{
		SetMovementState(EMovementState::Run);
	}
	else if (!bHasMoveInput && bCanRequestSprintStopFromRecentExit && CanRequestSprintStop())
	{
		RequestSprintStop();
	}
}

void ABAPlayerCharacter::StartSprintStopRequestWindow()
{
	bCanRequestSprintStopFromRecentExit = true;
	SprintStopRequestWindowRemainingTime = FMath::Max(0.f, SprintStopRequestWindowTime);
}

void ABAPlayerCharacter::UpdateSprintStopRequestWindow(const float DeltaTime)
{
	if (!bCanRequestSprintStopFromRecentExit)
	{
		return;
	}

	SprintStopRequestWindowRemainingTime -= DeltaTime;
	if (SprintStopRequestWindowRemainingTime <= 0.f)
	{
		bCanRequestSprintStopFromRecentExit = false;
		SprintStopRequestWindowRemainingTime = 0.f;

		if (!bSprintStopRequested)
		{
			ApplyLocomotionMovementPolicy();
		}
	}
}

bool ABAPlayerCharacter::CanRequestSprintStop() const
{
	return !bSprintStopRequested && GetGroundSpeed() >= SprintStopMinSpeed;
}

bool ABAPlayerCharacter::ShouldUseSprintMovementPolicy() const
{
	return CurrentMovementState == EMovementState::Sprint
		|| bSprintStopRequested
		|| bSprintStopMovementLocked
		|| bTurnaroundQueuedAfterSprintStop
		|| bCanBeginTurnaroundAfterSprintStop
		|| bCanRequestSprintStopFromRecentExit;
}

void ABAPlayerCharacter::UpdateTurnaroundRotation(const float DeltaTime)
{
	if (!bTurnaroundRequested)
	{
		return;
	}

	TurnaroundElapsedTime += DeltaTime;

	if (TurnaroundElapsedTime >= TurnaroundMaxDuration)
	{
		CompleteTurnaroundAnimation();
	}
}

void ABAPlayerCharacter::TickLadderClimb(float DeltaTime)
{
	if (!CurrentLadder.IsValid())
	{
		ExitLadder(GetActorLocation());
		return;
	}

	AMapLadder* Ladder = CurrentLadder.Get();
	const float VInput = MoveInputVector.Y;  // W/S
	const bool bSprintHeld = (CurrentMovementState == EMovementState::Sprint);

	float Speed = 0.f;
	if (VInput > KINDA_SMALL_NUMBER)
	{
		// 위로: Sprint면 빠르게
		Speed = bSprintHeld ? LadderClimbSpeedFast : LadderClimbSpeedSlow;
	}
	else if (VInput < -KINDA_SMALL_NUMBER)
	{
		// 아래로: Sprint면 슬라이드 다운
		Speed = bSprintHeld ? -LadderSlideDownSpeed : -LadderClimbSpeedSlow;
	}

	// Sprint 시 스태미너 소비 (위/아래 모두). 고갈되면 Run으로 강등
	if (bSprintHeld && FMath::Abs(VInput) > KINDA_SMALL_NUMBER)
	{
		ConsumeSprintStamina(DeltaTime);
		if (!CanSprint())
		{
			SetMovementState(EMovementState::Run);
		}
	}

	if (FMath::Abs(Speed) > KINDA_SMALL_NUMBER)
	{
		// sweep=false: 사다리/캡슐 충돌로 막히지 않게 직접 이동
		AddActorWorldOffset(FVector(0.f, 0.f, Speed * DeltaTime), false);
	}

	// 자동 이탈 — 이동 방향과 일치할 때만 (진입 직후 즉시 트리거 방지)
	const FVector Pos = GetActorLocation();
	const FVector TopLoc = Ladder->GetTopEntryLocation();
	const FVector BotLoc = Ladder->GetBottomEntryLocation();

	if (Speed > 0.f && Pos.Z >= TopLoc.Z)
	{
		// 위로 올라가다가 상단 도달 — 사다리 너머(forward) 약간 밀어내고 이탈
		const FVector Exit = TopLoc + GetActorForwardVector() * LadderExitClearance;
		ExitLadder(Exit);
	}
	else if (Speed < 0.f && Pos.Z <= BotLoc.Z)
	{
		// 아래로 내려가다가 하단 도달 — BottomEntry 위치로
		ExitLadder(BotLoc);
	}
}

void ABAPlayerCharacter::OnHealthChanged(float CurrentHP, float MaxHP)
{
	UUserDataSubsystem* UserData = GetGameInstance()->GetSubsystem<UUserDataSubsystem>();
	if (UserData && StatComponent)
	{
		// 변경 된 HP및 현 시점의 스테미너 수치를 전달
		UserData->NotifyPlayerStatChanged(CurrentHP, MaxHP, StatComponent->GetCurrentStamina(), StatComponent->GetMaxStamina());
	}
}

void ABAPlayerCharacter::OnStaminaChanged(float CurrentStamina, float MaxStamina)
{
	UUserDataSubsystem* UserData = GetGameInstance()->GetSubsystem<UUserDataSubsystem>();
	if (UserData && StatComponent)
	{	
		// 변경된 Stamina및 현 시점의 체력 수치를 전달
		UserData->NotifyPlayerStatChanged(StatComponent->GetCurrentHP(), StatComponent->GetMaxHP(), CurrentStamina, MaxStamina);
	}
}
