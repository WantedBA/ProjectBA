#include "Enemy/EnemyBase.h"
#include "Enemy/AI/EnemyAIController.h"
#include "Component/StatComponent.h"
#include "Component/CombatComponent.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BrainComponent.h"

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

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 360.f);
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
		return;
	}

	if (const FMonsterRows* MonsterRow = TableManager->FindMonster(InTid))
	{
		EnemyGrade = static_cast<EEnemyGrade>(MonsterRow->GradeType);

		if (StatComponent)
		{
			StatComponent->InitializeStats(
				static_cast<float>(MonsterRow->MaxHp),
				static_cast<float>(MonsterRow->Attack),
				static_cast<float>(MonsterRow->Defence)
			);
		}

		DetectRange = static_cast<float>(MonsterRow->DetectRange);

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
			GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

			MaxMoveSpeed = static_cast<float>(MonsterRow->MoveSpeed);
			GetCharacterMovement()->MaxWalkSpeed =MaxMoveSpeed;
		}

		USkeletalMesh* LoadedMesh = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, *MonsterRow->MeshPath));
		if (LoadedMesh)
		{
			GetMesh()->SetSkeletalMesh(LoadedMesh);
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
			ApplyKnockback(DamageCauser, 600.f);
			AAIController* AICon = Cast<AAIController>(GetController());
			if (AICon)
			{
				AICon->StopMovement();
			}
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
		TargetSpeed = MaxMoveSpeed;
		break;

	case EEnemyState::Move:
		TargetSpeed = MaxMoveSpeed * 0.8f;
		break;

	case EEnemyState::Attack:
	case EEnemyState::Hit:
	case EEnemyState::Dead:
		TargetSpeed = 0.f;
		break;

	default:
		TargetSpeed = MaxMoveSpeed;
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
}

void AEnemyBase::UpdateBlackBoardState()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController == nullptr)
	{
		return;
	}

	UBlackboardComponent* BBComp = AIController->GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return;
	}

	BBComp->SetValueAsEnum(BBKey::EnemyState, static_cast<uint8>(CurrentState));

	bool bIsActionLocked = (CurrentState == EEnemyState::Attack ||
		CurrentState == EEnemyState::Hit ||	CurrentState == EEnemyState::Dead);
	BBComp->SetValueAsBool(BBKey::IsActionLocked, bIsActionLocked);
	UE_LOG(LogTemp, Warning, TEXT("IsActionLocked : %s"), bIsActionLocked ? TEXT("True") : TEXT("False"));
}

void AEnemyBase::OnEnemyAttackAniFinished(EEnemyState NewState)
{
	if (IsValid(this) && CurrentState != EEnemyState::Dead)
	{
		SetState(EEnemyState::Idle);
	}

	UE_LOG(LogTemp, Warning, TEXT("Attack Finished -> %d"),(uint8)NewState);
	OnAttackAnimationFinished.Broadcast(NewState);
}

void AEnemyBase::OnDeath()
{
	Super::OnDeath();
	SetState(EEnemyState::Dead);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	SetActorEnableCollision(false);

	OnDeathEvent.Broadcast();
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->BrainComponent->StopLogic(TEXT("Dead"));
	}

	K2_OnDeadVisuals();
}

void AEnemyBase::SetState(EEnemyState NewState)
{
	if (CurrentState == NewState || (CurrentState == EEnemyState::Dead && NewState != EEnemyState::Dead))
	{
		return;
	}

	EEnemyState OldState = CurrentState;
	UE_LOG(LogTemp, Warning, TEXT("State Change %d -> %d"),(uint8)CurrentState,(uint8)NewState);
	CurrentState = NewState;

	UpdateBlackBoardState();
	UpdateMoveSpeed(CurrentState);

	OnStateChanged.Broadcast(OldState, NewState);
}

void AEnemyBase::ApplyKnockback(AActor* DamageCauser, float Force)
{
	if (DamageCauser == nullptr || bIsSuperArmor || IsDead())
	{
		return;
	}

	FVector KnockbackDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
	KnockbackDir.Z = 0.0f; // 수평 넉백

	FVector FinalForce = (KnockbackDir * Force);
	
	LaunchCharacter(FinalForce, true, true);
}

void AEnemyBase::Attack()
{
	if (IsDead())
	{
		return;
	}

	if (CurrentState == EEnemyState::Attack)
	{
		return;
	}

	SetState(EEnemyState::Attack);
	if (CombatComponent && AttackMontage)
	{
		CombatComponent->ExecuteAttack(AttackMontage);
	}
}