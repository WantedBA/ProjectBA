#include "Player/BAPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Component/StatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Instance/UserDataSubsystem.h"

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
	// static ConstructorHelpers::FClassFinder<UAnimInstance> CharacterAnim(TEXT("/Game/Character/Animation/ABP_ABCharacter.ABP_ABCharacter_C"));
	// if (CharacterAnim.Succeeded())
	// {
	// 	GetMesh()->SetAnimInstanceClass(CharacterAnim.Class);
	// }
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

	// 스탯 컴포넌트 생성
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));

	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.f, 0.f, -90.f),
		FRotator(0.f, -90.f, 0.f)
	);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

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
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void ABAPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializeFromTable();
	SetMovementState(EMovementState::Run);

	// StatComponent가 존재할 경우 변경된 델리게이트에 핸들러 함수 바인딩
	if (StatComponent)
	{
		// HP
		StatComponent->OnHPChanged.AddDynamic(this, &ABAPlayerCharacter::OnHealthChanged);

		// Stamina
		StatComponent->OnStaminaChanged.AddDynamic(this, &ABAPlayerCharacter::OnStaminaChanged);
	}

}

void ABAPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentMovementState != EMovementState::Sprint)
	{
		UpdateSprintExhaustionLock();
		return;
	}

	UpdateSprintExhaustionLock();
	LockSprintIfExhausted();

	if (!IsSprintMovementActive() || !CanSprint())
	{
		if (GEngine)
		{
			const FString Reason = !IsSprintMovementActive() ? TEXT("NoMoveInput") : TEXT("CannotSprint");
			GEngine->AddOnScreenDebugMessage(
				SprintStopDebugMessageKey,
				1.5f,
				FColor::Red,
				FString::Printf(TEXT("[Sprint Stop] Reason=%s Stamina=%.2f Min=%.2f HasData=%s Cost=%.2f Locked=%s"),
					*Reason,
					StatComponent ? StatComponent->GetCurrentStamina() : -1.f,
					SprintMinRequiredStamina,
					bHasSprintActionData ? TEXT("true") : TEXT("false"),
					SprintStaminaCost,
					bSprintLockedAfterExhausted ? TEXT("true") : TEXT("false"))
			);
		}
		SetMovementState(EMovementState::Run);
		return;
	}

	ConsumeSprintStamina(DeltaTime);

	if (!CanSprint())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				SprintStopDebugMessageKey,
				1.5f,
				FColor::Red,
				FString::Printf(TEXT("[Sprint Stop] Stamina exhausted. Stamina=%.2f Min=%.2f"),
					StatComponent ? StatComponent->GetCurrentStamina() : -1.f,
					SprintMinRequiredStamina)
			);
		}
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

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				SprintDebugMessageKey,
				3.f,
				FColor::Cyan,
				FString::Printf(TEXT("[Sprint Data] Tid=%d Cost=%.2f MinRequired=%.2f Restart=%.2f%%"),
					SprintActionTid,
					SprintStaminaCost,
					SprintMinRequiredStamina,
					SprintRestartStaminaPercent)
			);
		}
	}
	else
	{
		SprintStaminaCost = 0.f;
		SprintStaminaCostType = EPlayerStaminaCostType::Instant;
		SprintMinRequiredStamina = 0.f;
		SprintRestartStaminaPercent = 0.f;
		bHasSprintActionData = false;
		UE_LOG(LogTemp, Warning, TEXT("[BAPlayerCharacter] Sprint ActionData not found. Tid=%d"), SprintActionTid);
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

	if (CurrentMovementState == NewState)
	{
		return;
	}

	CurrentMovementState = NewState;
	
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

	const FString DebugText = FString::Printf(TEXT("[Sprint Consume] Cost=%.2f Delta=%.3f Consume=%.3f Stamina %.2f -> %.2f Locked=%s"),
		SprintStaminaCost,
		DeltaTime,
		ConsumeAmount,
		CurrentStamina,
		StatComponent->GetCurrentStamina(),
		bSprintLockedAfterExhausted ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Log, TEXT("%s"), *DebugText);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			SprintDebugMessageKey,
			0.1f,
			FColor::Cyan,
			DebugText
		);
	}
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

void ABAPlayerCharacter::SetHasMoveInput(const bool bNewHasMoveInput)
{
	bHasMoveInput = bNewHasMoveInput;

	if (!bHasMoveInput && CurrentMovementState == EMovementState::Sprint)
	{
		SetMovementState(EMovementState::Run);
	}
}
