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
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	CurrentState = EEnemyState::Idle;
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 360.f);

	MaxAttackCount = 3;
	AlertDuration = 3.0f;
	bShowDebugRanges = true;
}

void AEnemyBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (StatComponent)
	{
		StatComponent->OnDead.AddDynamic(this, &AEnemyBase::OnDeath);
	}

	OnAttackPerfectGuarded.AddUObject(this, &AEnemyBase::HandleAttackPerfectGuarded);
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
			bInitAI = true;
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
		AttackRange = static_cast<float>(MonsterRow->AttackRange);
		MaxChaseDistance = DetectRange * 1.5f;

		// 0인 경우 '무한' 또는 '항상 인지'로 처리 (매직넘버 방지)
		if (EnemyGrade == EEnemyGrade::Boss)
		{
			if (DetectRange <= 0.0f) DetectRange = 99999.0f;
			if (AttackRange <= 0.0f) AttackRange = 99999.0f; // 실제 공격 패턴 범위는 별도 계산되므로 추적용
			MaxChaseDistance = 99999.0f;
		}

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
			GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

			MaxMoveSpeed = static_cast<float>(MonsterRow->MoveSpeed);
			GetCharacterMovement()->MaxWalkSpeed = MaxMoveSpeed;
		}

		USkeletalMesh* LoadedMesh = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, *MonsterRow->MeshPath));
		if (LoadedMesh)
		{
			GetMesh()->SetSkeletalMesh(LoadedMesh);
		}
	}

	if (bInitAI == false)
	{
		AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController());
		if (AIController)
		{
			UE_LOG(LogTemp, Log, TEXT("[%s] Reinitialize AI after table init"), *GetName());
			AIController->InitializeAI(MonsterTid, this);
		}
	}
}

void AEnemyBase::OnDamaged(
	const float FinalDamage,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::OnDamaged(FinalDamage, DamageEvent, EventInstigator, DamageCauser);

	if (StatComponent)
	{
		StatComponent->ApplyDamage(FinalDamage);
		if (IsDead() == false)
		{
			// 슈퍼 아머가 아닐 때만 피격 상태로 전환
			if (bIsSuperArmor == false)
			{
				SetState(EEnemyState::Hit);
				ApplyKnockback(DamageCauser, 600.f);
			}

			AAIController* AICon = Cast<AAIController>(GetController());
			if (AICon)
			{
				// 피격 시 현재 경로 이동 중단
				AICon->StopMovement();
			}
		}
	}

	const FHitResult HitResult = ResolveDamageHitResult(DamageEvent);
	const FVector HitVisualLocation = HitResult.bBlockingHit ? FVector(HitResult.ImpactPoint) : GetActorLocation();
	K2_OnHitVisuals(HitVisualLocation);
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowDebugRanges)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), DetectRange, 32, FColor::Green, false, -0.1f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange, 32, FColor::Red, false, -0.1f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), GetActorLocation(), MaxChaseDistance, 32, FColor::Blue, false, -0.1f, 0, 1.0f);

		// Distance를 글자로 표기하자
		//AAIController* AIController = Cast<AAIController>(GetController());
		//if (AIController == nullptr)
		//{
		//	return;
		//}

		//UBlackboardComponent* BBComponent = AIController->GetBlackboardComponent();
		//if (BBComponent == nullptr)
		//{
		//	return;
		//}

		//float CurrentDistance = BBComponent->GetValueAsFloat(BBKey::TargetDistance);
		//AActor* Target = BBComponent ? Cast<AActor>(BBComponent->GetValueAsObject(BBKey::TargetActor)) : nullptr;
		//if (Target == nullptr)
		//{
		//	return;
		//}

		//float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		//FString DebugInfo = FString::Printf(TEXT("\nTargetDist: %.1f \nIdealRange: %.1f"), Dist, AttackRange);
		//float speed = GetCharacterMovement()->MaxWalkSpeed;
		//DebugInfo += FString::Printf(TEXT("\nMaxSpeed: %.1f"), speed);
		//DrawDebugString(GetWorld(), FVector(0, 0, 150), DebugInfo, this, FColor::Red, DeltaTime);
	}
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
	case EEnemyState::Idle:
		TargetSpeed = MaxMoveSpeed * 0.1f;
		break;

	case EEnemyState::Chase:
		// 추격은 걷기 모션으로 — 속도 기반 BlendSpace가 Run 대신 Walk를 고르도록 낮춘다.
		// 0.5 배율은 임시값. ABP BlendSpace의 Walk/Run 경계 속도에 맞춰 조정할 것.
		TargetSpeed = MaxMoveSpeed * 0.5f;
		break;

	case EEnemyState::Move:
		TargetSpeed = MaxMoveSpeed * 0.6f;
		break;

	case EEnemyState::Tactical:
		TargetSpeed = MaxMoveSpeed * 0.5f; // 서성일 때는 평소보다 느리게
		break;

	case EEnemyState::Alert:
		TargetSpeed = MaxMoveSpeed * 0.4f; // 경계 시 느리게 Strafe
		break;

	case EEnemyState::Attack:
	case EEnemyState::Hit:
	case EEnemyState::Stagger:
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

	// ActionLock 조건: 공격, 피격, 경직, 사망 상태일 때 AI 행동을 잠금
	bool bIsActionLocked = (CurrentState == EEnemyState::Attack || CurrentState == EEnemyState::Hit || CurrentState == EEnemyState::Stagger || CurrentState == EEnemyState::Dead);
	BBComp->SetValueAsBool(BBKey::IsActionLocked, bIsActionLocked);

	// [중요] IsInterrupted: 피격/경직만 포함 (상단 브랜치 강제 중단용)
	bool bIsInterrupted = (CurrentState == EEnemyState::Hit || CurrentState == EEnemyState::Stagger);
	BBComp->SetValueAsBool(BBKey::IsInterrupted, bIsInterrupted);
}

void AEnemyBase::ApplyKnockback(AActor* DamageCauser, float Force)
{
	if (DamageCauser == nullptr || GetCharacterMovement() == nullptr)
	{
		return;
	}

	// 넉백 방향 계산 (공격자 -> 몬스터 평면 방향)
	FVector KnockbackDirection = GetActorLocation() - DamageCauser->GetActorLocation();
	KnockbackDirection.Z = 0.0f; // Z축 공중 날아감 방지 (필요 시 약간 띄우려면 값을 추가)
	if (KnockbackDirection.IsNearlyZero() == false)
	{
		KnockbackDirection.Normalize();
	}
	else
	{
		// 완벽히 겹쳐있을 경우를 대비해 몬스터의 후방을 기본 방향으로 설정
		KnockbackDirection = -GetActorForwardVector();
	}

	// 최종 힘 계산 및 캐릭터 발사 (Launch)
	FVector LaunchVelocity = KnockbackDirection * Force;

	// 약간 위로 뜨는 넉백 느낌
	LaunchVelocity.Z = 20.0f; 

	// bXYOverride, bZOverride를 true로 주면 이전 이동 관성을 무시하고 즉시 밀려납니다.
	LaunchCharacter(LaunchVelocity, true, false);
}

void AEnemyBase::OnEnemyAttackAniFinished(EEnemyState NewState)
{
	if (IsValid(this) && CurrentState != EEnemyState::Dead)
	{
		// 공격 성공 시 횟수 증가 및 상태 판단
		CurrentAttackCount++;
		
		if (CurrentAttackCount >= MaxAttackCount)
		{
			SetState(EEnemyState::Alert);
		}
		else
		{
			SetState(EEnemyState::Idle);
		}
	}

	OnAttackAnimationFinished.Broadcast(NewState);
}

void AEnemyBase::OnDeath()
{
	Super::OnDeath();
	SetState(EEnemyState::Dead);

	SetActorEnableCollision(false);

	OnDeathEvent.Broadcast();

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->BrainComponent)
	{
		AIController->BrainComponent->StopLogic(TEXT("Dead"));
	}

	PlayAnimMontage(DeadMontage);

	K2_OnDeadVisuals();
}

void AEnemyBase::SetState(EEnemyState NewState)
{
	if (CurrentState == NewState || (CurrentState == EEnemyState::Dead && NewState != EEnemyState::Dead))
	{
		return;
	}

	// 기존 타이머가 있다면 취소 (새로운 상태가 우선)
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(StateTimerHandle);
	}

	EEnemyState OldState = CurrentState;
	CurrentState = NewState;

	// 애니메이션 및 복구 연동
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (CurrentState == EEnemyState::Hit || CurrentState == EEnemyState::Stagger)
	{
		if (AnimInstance)
		{
			// 현재 재생 중인 공격 몽타주 강제 중지
			if (AnimInstance->IsAnyMontagePlaying())
			{
				AnimInstance->Montage_Stop(0.2f);
			}

			// 상태에 맞는 리액션 몽타주 재생
			UAnimMontage* TargetMontage = (CurrentState == EEnemyState::Stagger) ? PerfectGuardedMontage : HitMontage;
			if (TargetMontage)
			{
				float Duration = PlayAnimMontage(TargetMontage);
				if (Duration > 0.0f)
				{
					// 애니메이션 종료 후 상태 복구 예약
					GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AEnemyBase::ResetStateToIdle, Duration, false);
				}
				else
				{
					// 몽타주 재생 실패 시 즉시 복구 (ActionLock 무한 루프 방지)
					ResetStateToIdle();
				}
			}
			else
			{
				ResetStateToIdle();
			}
		}
		else
		{
			ResetStateToIdle();
		}
	}
	else if (CurrentState == EEnemyState::Alert)
	{
		// 경계 상태 진입 시 일정 시간 후 Idle로 복구 (전투 템포 조절)
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AEnemyBase::ResetStateToIdle, AlertDuration, false);
	}

	// 상태 변화에 따른 이동 속도 및 블랙보드 갱신
	UpdateBlackBoardState();
	UpdateMoveSpeed(CurrentState);

	OnStateChanged.Broadcast(OldState, NewState);
}

void AEnemyBase::ResetStateToIdle()
{
	if (IsValid(this) && CurrentState != EEnemyState::Dead)
	{
		SetState(EEnemyState::Idle);
		ResetAttackCount(); // 공격 횟수 리셋하여 다시 공격 가능하게 함
		
		UE_LOG(LogTemp, Log, TEXT("[%s] State Recovered to Idle"), *GetName());
	}
}

bool AEnemyBase::CanAttack() const
{
	return CurrentAttackCount < MaxAttackCount && CurrentState != EEnemyState::Hit && CurrentState != EEnemyState::Stagger;
}

void AEnemyBase::HandleAttackPerfectGuarded(AActor* GuardingActor, const FHitResult& HitResult)
{
	if (!GuardingActor)
	{
		return;
	}

	HandlePerfectGuarded(HitResult.ImpactPoint);
}

void AEnemyBase::HandlePerfectGuarded(FVector ImpactLocation)
{
	if (IsDead())
	{
		return;
	}

	// 퍼펙트 가드 당할 시 '경직(Stagger)' 상태로 전환 (SetState 내에서 몽타주 재생 및 복구 타이머 처리됨)
	SetState(EEnemyState::Stagger);

	// 이펙트 및 피드백 재생
	if (PerfectDefenseVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PerfectDefenseVFX, ImpactLocation);
	}

	if (PerfectDefenseSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PerfectDefenseSFX, ImpactLocation);
	}

	if (PerfectDefenseCameraShake)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->ClientStartCameraShake(PerfectDefenseCameraShake);
		}
	}

	K2_OnPerfectGuarded(ImpactLocation);
}

void AEnemyBase::OnStartDissolve()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp == nullptr || DissolveMaterialsInput.Num() == 0)
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("StartDissolve Called!"));
	// 기존 배열 비우기
	DynamicDissolveMaterials.Empty();

	const int32 MaterialCount = MeshComp->GetNumMaterials();
	for (int32 i = 0; i < MaterialCount; ++i) // 머티리얼 교체 및 동적 인스턴스화
	{
		UMaterialInterface* SourceMat = nullptr;

		if (DissolveMaterialsInput.IsValidIndex(i))
		{
			SourceMat = DissolveMaterialsInput[i];
		}
		else
		{
			SourceMat = MeshComp->GetMaterial(i);
		}

		if (SourceMat == nullptr)
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMat = UMaterialInstanceDynamic::Create(SourceMat, this); // 동적 머티리얼 인스턴스 생성
		if (DynamicMat)
		{ 
			MeshComp->SetMaterial(i, DynamicMat); // 메시에 슬롯 번호(i) 맞춰서 교체
			DynamicDissolveMaterials.Add(DynamicMat); // 제어용 배열에 저장
		}
	}

	OnDissolveStarted.Broadcast();
}

void AEnemyBase::Attack()
{
	if (IsDead() || !CanAttack())
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
		float AttackDamage = 10.0f;
		if (StatComponent)
		{
			AttackDamage = StatComponent->GetAttack();
		}

		CombatComponent->SetAttackData(20.0f, AttackDamage);
		CombatComponent->ExecuteAttack(AttackMontage);
	}
}
