#include "Player/BAPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Component/StatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Instance/UserDataSubsystem.h"

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
	StatComponent->InitializeStats(
		BaseStat.MaxHp,	
		BaseStat.MaxStamina,
		BaseStat.StaminaRegenAmount,
		BaseStat.StaminaRegenDelay,
		BaseStat.WalkSpeed,
		BaseStat.RunSpeed,
		BaseStat.SprintSpeed,
		BaseStat.BaseAttack,
		BaseStat.BaseAttackSpeed,
		BaseStat.BaseDefence
	);

	GetCharacterMovement()->MaxWalkSpeed = BaseStat.RunSpeed;
}

void ABAPlayerCharacter::SetMovementState(EMovementState NewState)
{
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

void ABAPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// StatComponent가 존재할 경우 변경된 델리게이트에 핸들러 함수 바인딩
	if (StatComponent)
	{
		// HP
		StatComponent->OnHPChanged.AddDynamic(this, &ABAPlayerCharacter::OnHealthChanged);

		// Stamina
		StatComponent->OnStaminaChanged.AddDynamic(this, &ABAPlayerCharacter::OnStaminaChanged)

	}
}

void ABAPlayerCharacter::OnHealthChanged(float CurrentHP, float MaxHP)
{
	// 수신된 데이터를 UserDataSubsystem으로 전달
	UUserDataSubsystem* UserData = GetGameInstance()->GetSubsystem<UUserDataSubsystem>();
	if (UserData)
	{
		// UI 갱신을 위해 서브시스템의 알림 함수 호출
	}
}

void ABAPlayerCharacter::OnStaminaChanged(float CurrentStamina, float MaxStamina)
{
	UUserDataSubsystem User
}

