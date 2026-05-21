#include "Component/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/DamageEvents.h"
#include "Enemy/EnemyBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Instance/BATimeSubsystem.h"
#include "Component/ActionComponent.h"
#include "Player/BAPlayerCharacter.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsHitChecking)
	{
		ProcessHitCheck();
	}
}

void UCombatComponent::ExecuteAttack(UAnimMontage* AttackMontage, float PlayRate)
{
	if (AttackMontage == nullptr)
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerCharacter->PlayAnimMontage(AttackMontage, PlayRate);
	}
}

void UCombatComponent::CheckHitStartDefault()
{
	CheckHitStart(CurrentRadius, CurrentDamage, StartSocketName, EndSocketName);
}

void UCombatComponent::CheckHitStart(float InRadius, float InDamage, FName InStartSocket, FName InEndSocket)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	bIsHitChecking = true;
	CurrentRadius = InRadius;
	
	if (InDamage >= 0.0f)
	{
		CurrentDamage = InDamage;
	}

	if (InStartSocket != NAME_None)
	{
		StartSocketName = InStartSocket;
	}

	if (InEndSocket != NAME_None)
	{
		EndSocketName = InEndSocket;
	}
	
	PrevStartLocation = OwnerCharacter->GetMesh()->GetSocketLocation(StartSocketName);
	PrevEndLocation = OwnerCharacter->GetMesh()->GetSocketLocation(EndSocketName);
	HitActors.Empty();
	
	SetComponentTickEnabled(true);
}

void UCombatComponent::CheckHitEnd()
{
	bIsHitChecking = false;
	SetComponentTickEnabled(false);
}

void UCombatComponent::SetAttackData(
	float InRadius,
	float InDamage,
	FName InStartSocket,
	FName InEndSocket,
	EBADamageReactionType InDamageReactionType)
{
	CurrentRadius = InRadius;
	CurrentDamage = InDamage;
	CurrentDamageReactionType = InDamageReactionType;
	
	if (InStartSocket != NAME_None)
	{
		StartSocketName = InStartSocket;
	}

	if (InEndSocket != NAME_None)
	{
		EndSocketName = InEndSocket;
	}
}

void UCombatComponent::SetPerfectWindowActive(bool bActive, float TimeDilation, float Duration)
{
	bIsPerfectWindowActive = bActive;
	ActivePerfectTimeDilation = TimeDilation;
	ActivePerfectDuration = Duration;
}

void UCombatComponent::TriggerHitStop(float Duration)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	UBATimeSubsystem* TimeSubsystem = World->GetSubsystem<UBATimeSubsystem>();
	if (TimeSubsystem == nullptr)
	{
		return;
	}

	TimeSubsystem->ApplyHitStop(Duration);
}

void UCombatComponent::ProcessHitCheck()
{
	ACharacterBase* OwnerCharacter = Cast<ACharacterBase>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	FVector CurrentStart;
	FVector CurrentEnd;
	
	// WeaponMesh가 있으면 WeaponMesh, 없으면 GetMesh에서 소켓 탐색 (GetWeaponMesh Override 필요)
	if (OwnerCharacter->GetWeaponMesh())
	{
		CurrentStart = OwnerCharacter->GetWeaponMesh()->GetSocketLocation(StartSocketName);
		CurrentEnd = OwnerCharacter->GetWeaponMesh()->GetSocketLocation(EndSocketName);
	}
	else
	{
		CurrentStart = OwnerCharacter->GetMesh()->GetSocketLocation(StartSocketName);
		CurrentEnd = OwnerCharacter->GetMesh()->GetSocketLocation(EndSocketName);
	}
	
	// 무기의 중심점과 방향 계산
	FVector CurrentMid = (CurrentStart + CurrentEnd) * 0.5f;
	FVector PrevMid = (PrevStartLocation + PrevEndLocation) * 0.5f;
	
	// 무기의 길이 계산
	float WeaponLength = FVector::Dist(CurrentStart, CurrentEnd);
	
	// BoxTrace를 위한 설정 (두께는 CurrentRadius, 길이는 WeaponLength)
	// X축이 무기 방향이라고 가정
	FVector HalfSize = FVector(WeaponLength * 0.5f, CurrentRadius, CurrentRadius);
	FRotator Orientation = (CurrentEnd - CurrentStart).Rotation();

	TArray<FHitResult> OutHits;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	EDrawDebugTrace::Type DebugTrace = bShowDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	// Box Trace: 이전 위치에서 현재 위치까지 무기 전체를 스윕(Sweep)
	bool bHit = UKismetSystemLibrary::BoxTraceMulti(
		this,
		PrevMid,
		CurrentMid,
		HalfSize,
		Orientation,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		DebugTrace,
		OutHits,
		true
	);

	if (bHit)
	{
		for (const FHitResult& Hit : OutHits)
		{
			AActor* Victim = Hit.GetActor();
			if (Victim && !HitActors.Contains(Victim))
			{
				HitActors.Add(Victim);
				ApplyDamage(Victim, Hit);
			}
		}
	}

	PrevStartLocation = CurrentStart;
	PrevEndLocation = CurrentEnd;
}

void UCombatComponent::SpawnShockwave(FVector Location, float Scale)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// Shockwave Niagara
	if (ShockwaveSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			ShockwaveSystem,
			Location,
			FRotator::ZeroRotator,
			FVector(Scale),
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true
		);
	}

	// Distortion Niagara
	if (DistortionSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			DistortionSystem,
			Location,
			FRotator::ZeroRotator,
			FVector(Scale),
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true
		);
	}
}

void UCombatComponent::ApplyDamage(AActor* Victim, const FHitResult& HitResult)
{
	if (Victim == nullptr)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	AEnemyBase* OwnerEnemy = Cast<AEnemyBase>(OwnerActor);
	AController* Instigator = OwnerActor ? OwnerActor->GetInstigatorController() : nullptr;

	// 퍼펙트 가드/회피 체크 - Owner의 현재 공격이 퍼펙트 가드/회피를 허용하는가
	if (bIsPerfectWindowActive)
	{
		UActionComponent* VictimAction = Victim->FindComponentByClass<UActionComponent>();
		if (VictimAction)
		{
			// 피격 상대의 Dodged 상태와 Guard 상태 확인
			const EGuardState GuardState = VictimAction->GetGuardState();
			bool bPerfectGuarded = false;
			if (GuardState == EGuardState::Guarding || GuardState == EGuardState::Blocking)
			{
				if (ABAPlayerCharacter* VictimPlayer = Cast<ABAPlayerCharacter>(Victim))
				{
					bPerfectGuarded = VictimPlayer->TryConsumePerfectGuardStamina();
				}
				else
				{
					bPerfectGuarded = true;
				}
			}
			const bool bPerfectDodged = VictimAction->GetActionRuntimeState() == EActionRuntimeState::Dodging;

			if (bPerfectGuarded || bPerfectDodged)
			{
				// 슬로우 모션 발동
				if (UWorld* World = GetWorld())
				{
					if (UBATimeSubsystem* TimeSubsystem = World->GetSubsystem<UBATimeSubsystem>())
					{
						TimeSubsystem->ApplySlowMotion(ActivePerfectTimeDilation, ActivePerfectDuration);
					}
				}

				// 몬스터 리액션 및 피드백 실행 (VFX, SFX, CameraShake 포함) - Owner가 Enemy인 경우에만 적용
				if (OwnerEnemy)
				{
					OwnerEnemy->HandlePerfectGuarded(HitResult.ImpactPoint);
				}

				return; // 데미지 적용 취소
			}
		}
	}

	// 피격 반응 판단에 필요한 공격 강도와 방향을 TakeDamage 경계로 함께 전달한다.
	FBADamageEvent DamageEvent;
	DamageEvent.DamageReactionType = CurrentDamageReactionType;
	DamageEvent.HitResult = HitResult;

	if (OwnerActor)
	{
		DamageEvent.DamageDirection = (Victim->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal();
	}
	if (DamageEvent.DamageDirection.IsNearlyZero())
	{
		DamageEvent.DamageDirection = OwnerActor ? OwnerActor->GetActorForwardVector().GetSafeNormal() : FVector::ZeroVector;
	}

	Victim->TakeDamage(CurrentDamage, DamageEvent, Instigator, OwnerActor);

	// 충격파 및 왜곡 발생
	SpawnShockwave(HitResult.ImpactPoint, 1.0f);

	// 넉백 처리 - 상대가 Enemy인 경우에만 적용
	if (AEnemyBase* EnemyVictim = Cast<AEnemyBase>(Victim))
	{
		EnemyVictim->ApplyKnockback(OwnerActor, 500.0f);
	}

	if (OnHitDetected.IsBound())
	{
		OnHitDetected.Broadcast(Victim, HitResult);
	}
}
