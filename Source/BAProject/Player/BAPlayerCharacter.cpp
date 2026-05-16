#include "Player/BAPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Component/InteractorComponent.h"
#include "Component/StatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Instance/UserDataSubsystem.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// TODO: id 하드코딩
	constexpr int32 SprintActionTid = 10020;
	constexpr uint64 SprintDebugMessageKey = 12020;
	constexpr uint64 SprintStopDebugMessageKey = 12021;
	constexpr float DefaultSprintRestartStaminaPercent = 70.f;
}

// 플레이어 캐릭터의 기본 메시, 애니메이션, 컴포넌트, 카메라, 이동 기본값을 구성한다.
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
	GetCharacterMovement()->MaxWalkSpeed = SpeedSettings.RunSpeed;
}

// 데이터 초기화와 스탯 변경 이벤트 바인딩을 수행한다.
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

// Sprint, Sprint Stop, Turnaround 상태를 매 프레임 갱신하고 스태미나 소모를 처리한다.
void ABAPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSmoothedMoveInput(DeltaTime);
	UpdateSprintStopRequest(DeltaTime);
	UpdateSprintStopRequestWindow(DeltaTime);
	UpdateTurnaroundRotation(DeltaTime);

	if (MovementRuntime.MovementState != EMovementState::Sprint)
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

// 기본 공격 입력 진입점이며, 실제 공격 로직은 아직 연결되지 않았다.
void ABAPlayerCharacter::Attack()
{
	
}

// UserDataSubsystem의 기본 스탯과 Action 데이터를 플레이어 런타임 설정에 반영한다.
void ABAPlayerCharacter::InitializeFromTable()
{
	const UUserDataSubsystem* UserDataSubsystem = UUserDataSubsystem::Get(this);
	if (!UserDataSubsystem)
	{
		return;
	}

	const FPlayerBaseStat BaseStat = UserDataSubsystem->GetBaseStat();
	SpeedSettings.WalkSpeed = BaseStat.WalkSpeed;
	SpeedSettings.RunSpeed = BaseStat.RunSpeed;
	SpeedSettings.SprintSpeed = BaseStat.SprintSpeed;

	StatComponent->InitializeStats(
		BaseStat.MaxHp,	
		BaseStat.MaxStamina,
		BaseStat.StaminaRecoveryPerSecond,
		BaseStat.StaminaRecoveryDelay,
		SpeedSettings.WalkSpeed,
		SpeedSettings.RunSpeed,
		SpeedSettings.SprintSpeed,
		BaseStat.BaseAttack,
		BaseStat.BaseAttackSpeed,
		BaseStat.BaseDefence
	);

	if (const FPlayerActionData* SprintActionData = UserDataSubsystem->FindActionData(SprintActionTid))
	{
		SprintRuntime.StaminaCost = FMath::Max(0.f, SprintActionData->StaminaCost);
		SprintRuntime.StaminaCostType = SprintActionData->StaminaCostType;
		SprintRuntime.MinRequiredStamina = FMath::Max(0.f, SprintActionData->MinRequiredStamina);
		SprintRuntime.RestartStaminaPercent = SprintActionData->SprintRestartStaminaPercent > 0.f
			? FMath::Clamp(SprintActionData->SprintRestartStaminaPercent, 0.f, 100.f)
			: DefaultSprintRestartStaminaPercent;
		SprintRuntime.bHasActionData = true;
	}
	else
	{
		SprintRuntime.StaminaCost = 0.f;
		SprintRuntime.StaminaCostType = EPlayerStaminaCostType::Instant;
		SprintRuntime.MinRequiredStamina = 0.f;
		SprintRuntime.RestartStaminaPercent = 0.f;
		SprintRuntime.bHasActionData = false;
	}

	GetCharacterMovement()->MaxWalkSpeed = SpeedSettings.RunSpeed;
}
