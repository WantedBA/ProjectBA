#include "Enemy/EnemyBase.h"
#include "Enemy/AI/EnemyAIController.h"
#include "Component/StatComponent.h"
#include "Component/CombatComponent.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CurrentState = EEnemyState::Idle;
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
}

void AEnemyBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (StatComponent)
	{
		StatComponent->OnDead.AddDynamic(this, &AEnemyBase::OnDeath);
	}

	OnAnimationFinished.AddUObject(this, &AEnemyBase::OnEnemyAttackAniFinished);
}

void AEnemyBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AEnemyAIController* AIController = Cast<AEnemyAIController>(NewController);
	if (AIController)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] PossessedBy %s. MonsterTid: %d"), *GetName(), *NewController->GetName(), MonsterTid);
		if (MonsterTid != 0)
		{
			AIController->InitializeAI(MonsterTid, this);
		}
	}
}

void AEnemyBase::InitializeFromTable(int32 InTid)
{
	MonsterTid = InTid;

	UBATableManager* TableManager = UBATableManager::Get(this);
	if (TableManager == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("[AEnemyBase] Monster data not found for Tid: %d"), MonsterTid);
		return;
	}

	if (const FMonsterRows* MonsterRow = TableManager->FindMonster(InTid))
	{
		EnemyGrade = static_cast<EEnemyGrade>(MonsterRow->GradeType);

		StatComponent->InitializeStats(
			static_cast<float>(MonsterRow->MaxHp),
			static_cast<float>(MonsterRow->Attack),
			static_cast<float>(MonsterRow->Defence)
		);

		DetectRange = MonsterRow->DetectRange;

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = static_cast<float>(MonsterRow->MoveSpeed);
			GetCharacterMovement()->bOrientRotationToMovement = true;
			GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
		}

		if (!MonsterRow->MeshPath.IsEmpty())
		{
			if (USkeletalMesh* LoadedMesh = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, *MonsterRow->MeshPath)))
			{
				GetMesh()->SetSkeletalMesh(LoadedMesh);
			}
		}

		if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
		{
			AIController->InitializeAI(MonsterTid, this);
		}
	}
}

void AEnemyBase::OnDamaged(float FinalDamage, AActor* DamageCauser)
{
	Super::OnDamaged(FinalDamage, DamageCauser);

	if (StatComponent)
	{
		StatComponent->ApplyDamage(FinalDamage);
		if (IsDead() == false)
		{
			SetState(EEnemyState::Hit);
		}
	}

	K2_OnHitVisuals(GetActorLocation());
}

void AEnemyBase::UpdateMoveSpeed(EEnemyState NewState)
{
	if (GetCharacterMovement() == nullptr)
	{
		return;
	}

	float TargetSpeed = 0.0f;
	switch (NewState)
	{
	case EEnemyState::Chase:
		TargetSpeed = GetCharacterMovement()->MaxWalkSpeed; // 보스 데이터 테이블 혹은 상수로 정의된 값
		break;
	case EEnemyState::Move:
		TargetSpeed = GetCharacterMovement()->MaxWalkSpeed * 0.8f;
		break;
	default:
		TargetSpeed = 0.0f;
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
}

void AEnemyBase::OnEnemyAttackAniFinished(EEnemyState NewState)
{
	if (IsValid(this) && CurrentState != EEnemyState::Dead)
	{
		SetState(NewState);
	}
}

void AEnemyBase::OnDeath()
{
	Super::OnDeath();
	SetState(EEnemyState::Dead);
	OnDeathEvent.Broadcast();
	K2_OnDeadVisuals();
}

void AEnemyBase::SetState(EEnemyState NewState)
{
	if (CurrentState == NewState || (CurrentState == EEnemyState::Dead && NewState != EEnemyState::Dead))
	{
		return;
	}

	EEnemyState OldState = CurrentState;
	CurrentState = NewState;

	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
	{
		UBlackboardComponent* BBComponent = AIController->GetBlackboardComponent();
		if (BBComponent)
		{
			// 다음 행위 중에는 BT 막기
			bool bIsActionLocked = (CurrentState == EEnemyState::Attack || CurrentState == EEnemyState::Hit || CurrentState == EEnemyState::Dead);
			BBComponent->SetValueAsBool(BBKey::IsActionLocked, bIsActionLocked);
			UpdateMoveSpeed(CurrentState);
		}
	}

	OnStateChanged.Broadcast(OldState, NewState);
}

void AEnemyBase::Attack()
{
	if (IsDead()) return;

	SetState(EEnemyState::Attack);
	if (CombatComponent && AttackMontage)
	{
		CombatComponent->ExecuteAttack(AttackMontage);
	}
}