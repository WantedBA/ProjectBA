#include "Enemy/EnemyBase.h"
#include "Enemy/AI/EnemyAIController.h"
#include "Component/StatComponent.h"
#include "Component/CombatComponent.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
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
}

void AEnemyBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(NewController))
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
			GetCharacterMovement()->bOrientRotationToMovement = true; // �̵� �������� ĳ���� ȸ��
			GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f); // ȸ�� �ӵ� ����
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

void AEnemyBase::OnDeath()
{
	Super::OnDeath();
	SetState(EEnemyState::Dead);

	K2_OnDeadVisuals();
}

void AEnemyBase::SetState(EEnemyState NewState)
{
	if (CurrentState == NewState || IsDead())
	{
		return;
	}

	EEnemyState OldState = CurrentState;
	CurrentState = NewState;
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