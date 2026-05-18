#include "Player/BAPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Component/ActionAnimationComponent.h"
#include "Component/ActionComponent.h"
#include "Component/InteractorComponent.h"
#include "Component/StatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Instance/UserDataSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "Tables/BATableManager.h"

namespace
{
	// TODO: id 하드코딩
	constexpr int32 SprintActionTid = 10020;
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

	// 공용 액션 컴포넌트 생성
	ActionComponent = CreateDefaultSubobject<UActionComponent>(TEXT("ActionComponent"));

	// 공용 액션 애니메이션 재생 컴포넌트 생성
	ActionAnimationComponent = CreateDefaultSubobject<UActionAnimationComponent>(TEXT("ActionAnimationComponent"));

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

	GetCharacterMovement()->MaxWalkSpeed = SpeedSettings.RunSpeed;
}

// 데이터 초기화와 스탯 변경 이벤트 바인딩을 수행한다.
void ABAPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializeFromTable();
	SyncFreeStrafeFacingMode();
	SetActiveGaitAndSpeed(EMovementState::Run);

	if (StatComponent)
	{
		StatComponent->OnHPChanged.AddDynamic(this, &ABAPlayerCharacter::OnHealthChanged);
		StatComponent->OnStaminaChanged.AddDynamic(this, &ABAPlayerCharacter::OnStaminaChanged);

		OnHealthChanged(StatComponent->GetCurrentHP(), StatComponent->GetMaxHP());
		OnStaminaChanged(StatComponent->GetCurrentStamina(), StatComponent->GetMaxStamina());
	}

	if (ActionComponent)
	{
		ActionComponent->OnActionStarted.AddDynamic(this, &ABAPlayerCharacter::HandleActionStarted);
	}
}

// 공통 Movement 상태를 매 프레임 갱신하고, 가능한 경우 이동 입력을 소비한다.
void ABAPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsOnLadder())
	{
		TickLadderClimb(DeltaTime);
		return;
	}
	
	TickMovementRuntime(DeltaTime);
}

// 기본 공격 입력 진입점
void ABAPlayerCharacter::Attack()
{
}

// UserDataSubsystem의 기본 스탯과 공용 Action 데이터를 플레이어 런타임 설정에 반영한다.
void ABAPlayerCharacter::InitializeFromTable()
{
	const UUserDataSubsystem* UserDataSubsystem = UUserDataSubsystem::Get(this);
	if (!UserDataSubsystem || !StatComponent)
	{
		return;
	}

	const FPlayerBaseStat BaseStat = UserDataSubsystem->GetBaseStat();
	SpeedSettings.WalkSpeed = BaseStat.WalkSpeed;
	SpeedSettings.RunSpeed = BaseStat.RunSpeed;
	SpeedSettings.SprintSpeed = BaseStat.SprintSpeed;

	// 사다리 관련 스탯 초기화
	LadderSettings.ClimbSpeedSlow = BaseStat.LadderClimbSpeedSlow;
	LadderSettings.ClimbSpeedFast = BaseStat.LadderClimbSpeedFast;
	LadderSettings.SlideDownSpeed = BaseStat.LadderSlideDownSpeed;
	LadderSettings.ExitClearance = BaseStat.LadderExitClearance;

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

	const UBATableManager* TableManager = UBATableManager::Get(this);
	if (const FActionDataRow* SprintActionData = TableManager ? TableManager->FindActionData(SprintActionTid) : nullptr)
	{
		SprintCostSettings.StaminaCost = FMath::Max(0.f, SprintActionData->StaminaCost);
		SprintCostSettings.StaminaCostType = SprintActionData->StaminaCostType;
		SprintCostSettings.MinRequiredStamina = FMath::Max(0.f, SprintActionData->MinRequiredStamina);
		SprintCostSettings.RestartStaminaPercent = SprintActionData->SprintRestartStaminaPercent > 0.f
			? FMath::Clamp(SprintActionData->SprintRestartStaminaPercent, 0.f, 100.f)
			: DefaultSprintRestartStaminaPercent;
		SprintCostSettings.bHasActionData = true;
	}
	else
	{
		SprintCostSettings.StaminaCost = 0.f;
		SprintCostSettings.StaminaCostType = EActionStaminaCostType::Instant;
		SprintCostSettings.MinRequiredStamina = 0.f;
		SprintCostSettings.RestartStaminaPercent = DefaultSprintRestartStaminaPercent;
		SprintCostSettings.bHasActionData = false;
	}

	GetCharacterMovement()->MaxWalkSpeed = SpeedSettings.RunSpeed;
}

void ABAPlayerCharacter::HandleActionStarted(const int32 ActionTid, const EActionType ActionType)
{
	if (!ActionComponent || !ActionComponent->IsMovementLockedByAction())
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	MovementRuntime.Phase = EPlayerMovementPhase::None;
	MovementRuntime.PhaseElapsedTime = 0.f;
	MovementRuntime.bWaitingForPhaseAnimation = false;
}
