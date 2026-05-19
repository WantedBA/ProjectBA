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

void UCombatComponent::SetAttackData(float InRadius, float InDamage, FName InStartSocket, FName InEndSocket)
{
	CurrentRadius = InRadius;
	CurrentDamage = InDamage;
	
	if (InStartSocket != NAME_None)
	{
		StartSocketName = InStartSocket;
	}

	if (InEndSocket != NAME_None)
	{
		EndSocketName = InEndSocket;
	}
}

void UCombatComponent::TriggerHitStop(float Duration)
{
	if (UWorld* World = GetWorld())
	{
		if (UBATimeSubsystem* TimeSubsystem = World->GetSubsystem<UBATimeSubsystem>())
		{
			TimeSubsystem->ApplyHitStop(Duration);
		}
	}
}

void UCombatComponent::ProcessHitCheck()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	FVector CurrentStart = OwnerCharacter->GetMesh()->GetSocketLocation(StartSocketName);
	FVector CurrentEnd = OwnerCharacter->GetMesh()->GetSocketLocation(EndSocketName);
	
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

#include "Instance/BATimeSubsystem.h"
#include "Component/ActionComponent.h"

void UCombatComponent::ApplyDamage(AActor* Victim, const FHitResult& HitResult)
{
	if (Victim == nullptr)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	AEnemyBase* OwnerEnemy = Cast<AEnemyBase>(OwnerActor);
	AController* Instigator = OwnerActor ? OwnerActor->GetInstigatorController() : nullptr;

	// 퍼펙트 가드/회피 체크
	if (bIsPerfectWindowActive)
	{
		UActionComponent* VictimAction = Victim->FindComponentByClass<UActionComponent>();
		if (VictimAction)
		{
			bool bPerfectGuarded = VictimAction->GetGuardState() != EGuardState::None;
			bool bPerfectDodged = VictimAction->GetActionRuntimeState() == EActionRuntimeState::Dodging;

			if (bPerfectGuarded || bPerfectDodged)
			{
				// 슬로우 모션 발동
				if (UWorld* World = GetWorld())
				{
					if (UBATimeSubsystem* TimeSubsystem = World->GetSubsystem<UBATimeSubsystem>())
					{
						TimeSubsystem->ApplySlowMotion(0.1f, 0.5f);
					}
				}

				// 몬스터 리액션 및 피드백 실행 (VFX, SFX, CameraShake 포함)
				if (OwnerEnemy)
				{
					OwnerEnemy->HandlePerfectGuarded(HitResult.ImpactPoint);
				}

				return; // 데미지 적용 취소
			}
		}
	}

	FDamageEvent DamageEvent;
	Victim->TakeDamage(CurrentDamage, DamageEvent, Instigator, OwnerActor);

	// 충격파 및 왜곡 발생
	SpawnShockwave(HitResult.ImpactPoint, 1.0f);

	// 넉백 처리
	if (AEnemyBase* EnemyVictim = Cast<AEnemyBase>(Victim))
	{
		EnemyVictim->ApplyKnockback(OwnerActor, 500.0f);
	}

	if (OnHitDetected.IsBound())
	{
		OnHitDetected.Broadcast(Victim, HitResult);
	}
}
