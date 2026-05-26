#include "Component/CombatComponent.h"
#include "Character/CharacterBase.h"
#include "Constants/BAProjectConstant.h"
#include "Enemy/EnemyBase.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/DamageEvents.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Enemy/Monster.h"

/*
 * Combat Damage Direction Policy Summary
 *
 * CombatComponent는 히트 판정 결과를 피해 이벤트로 정리한다. 피격자의 상태 변경과 애니메이션 선택은
 * 피격자 쪽 Damage 모듈이 담당하고, 여기서는 피해량, 피격 타입, 런치 세기, 방향만 넘긴다.
 *
 * 1. 공격 데이터
 * - SetAttackData는 노티파이에서 사용할 현재 공격의 반경, 피해량, 피격 타입, 런치 속도를 저장한다.
 * - 보스 패턴은 테이블의 DamageReactionType, LaunchHorizontalSpeed, LaunchVerticalSpeed를 그대로 넘긴다.
 * - Launch 값이 0이면 피격자가 가진 기본값을 사용한다.
 *
 * 2. 방향 계산
 * - 기본 DamageDirection은 공격자 중심에서 피격자 중심으로 향하는 평면 방향이다.
 * - 이 방향은 가드 각도, 피격 방향 몽타주, 플레이어 런치 방향에 함께 쓰인다.
 * - 평면 방향이 없으면 공격자의 전방 방향을 사용한다.
 *
 * 3. KnockDown 보정
 * - KnockDown은 수평 런치가 중요하므로, 캡슐끼리 너무 가까우면 중심 방향이 불안정하다.
 * - 공격자/피격자 캡슐 반지름 합에 KnockDownDirectionContactTolerance를 더한 거리 안에서는 공격자 전방 방향을 사용한다.
 * - 이 보정은 KnockDown에만 적용한다. 일반 피격과 가드 판정의 방향 규칙은 바꾸지 않는다.
 */
namespace
{
	FVector GetFlatSafeDirection(const FVector& Direction)
	{
		FVector FlatDirection = Direction;
		FlatDirection.Z = 0.f;
		return FlatDirection.GetSafeNormal();
	}

	float GetCharacterCapsuleRadius(const AActor* Actor)
	{
		const ACharacter* Character = Cast<ACharacter>(Actor);
		const UCapsuleComponent* CapsuleComponent = Character ? Character->GetCapsuleComponent() : nullptr;
		return CapsuleComponent ? CapsuleComponent->GetScaledCapsuleRadius() : 0.f;
	}

	bool ShouldUseOwnerForwardForKnockDownDirection(
		const AActor* OwnerActor,
		const AActor* Victim,
		const FVector& OwnerToVictim,
		const float ContactTolerance)
	{
		if (!OwnerActor || !Victim)
		{
			return false;
		}

		const FVector FlatOwnerToVictim = FVector(OwnerToVictim.X, OwnerToVictim.Y, 0.f);
		if (FlatOwnerToVictim.IsNearlyZero())
		{
			return true;
		}

		const float CombinedCapsuleRadius = GetCharacterCapsuleRadius(OwnerActor) + GetCharacterCapsuleRadius(Victim);
		if (CombinedCapsuleRadius <= 0.f)
		{
			return false;
		}

		return FlatOwnerToVictim.Size() <= CombinedCapsuleRadius + FMath::Max(0.f, ContactTolerance);
	}

	ECollisionChannel ResolveAttackTraceChannel(const ACharacterBase& OwnerCharacter)
	{
		return OwnerCharacter.IsA<AEnemyBase>()
			? CollisionChannel::EnemyAttackTrace
			: CollisionChannel::PlayerAttackTrace;
	}

	bool IsEnemyFriendlyFire(const AActor* OwnerActor, const AActor* Victim)
	{
		return OwnerActor
			&& Victim
			&& OwnerActor->IsA<AEnemyBase>()
			&& Victim->IsA<AEnemyBase>();
	}
}

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

	ResetTargetHitRecords();

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
	ACharacterBase* OwnerCharacter = Cast<ACharacterBase>(GetOwner());
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
	
	// Prev 위치는 ProcessHitCheck와 반드시 같은 메시(무기 우선)에서 가져와야 한다.
	// 안 그러면 첫 히트체크 프레임의 스윕이 본체↔무기 사이를 가로질러 거대한 박스가 된다.
	if (OwnerCharacter->GetWeaponMesh())
	{
		PrevStartLocation = OwnerCharacter->GetWeaponMesh()->GetSocketLocation(StartSocketName);
		PrevEndLocation = OwnerCharacter->GetWeaponMesh()->GetSocketLocation(EndSocketName);
	}
	else
	{
		PrevStartLocation = OwnerCharacter->GetMesh()->GetSocketLocation(StartSocketName);
		PrevEndLocation = OwnerCharacter->GetMesh()->GetSocketLocation(EndSocketName);
	}
	
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
	EBADamageReactionType InDamageReactionType,
	float InLaunchHorizontalSpeed,
	float InLaunchVerticalSpeed)
{
	ResetTargetHitRecords();

	CurrentRadius = InRadius;
	CurrentDamage = InDamage;
	CurrentDamageReactionType = InDamageReactionType;
	CurrentLaunchHorizontalSpeed = FMath::Max(0.f, InLaunchHorizontalSpeed);
	CurrentLaunchVerticalSpeed = FMath::Max(0.f, InLaunchVerticalSpeed);
	
	if (InStartSocket != NAME_None)
	{
		StartSocketName = InStartSocket;
	}

	if (InEndSocket != NAME_None)
	{
		EndSocketName = InEndSocket;
	}
}

void UCombatComponent::SetMaxHitsPerTargetPerAttack(const int32 InMaxHits)
{
	MaxHitsPerTargetPerAttack = FMath::Max(0, InMaxHits);
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
	const ECollisionChannel HitTraceChannel = ResolveAttackTraceChannel(*OwnerCharacter);

	// Box Trace: 이전 위치에서 현재 위치까지 무기 전체를 스윕(Sweep)
	bool bHit = UKismetSystemLibrary::BoxTraceMulti(
		this,
		PrevMid,
		CurrentMid,
		HalfSize,
		Orientation,
		UEngineTypes::ConvertToTraceType(HitTraceChannel),
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
			if (CanRegisterHit(Victim))
			{
				RegisterTargetHit(Victim);
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

void UCombatComponent::ResetTargetHitRecords()
{
	TargetHitRecords.Empty();
}

bool UCombatComponent::CanRegisterHit(AActor* Victim) const
{
	if (Victim == nullptr)
	{
		return false;
	}

	if (Victim == GetOwner() || IsEnemyFriendlyFire(GetOwner(), Victim))
	{
		return false;
	}

	if (MaxHitsPerTargetPerAttack <= 0)
	{
		return true;
	}

	const FCombatTargetHitRecord* HitRecord = FindHitRecord(Victim);
	return HitRecord == nullptr || HitRecord->HitCount < MaxHitsPerTargetPerAttack;
}

void UCombatComponent::RegisterTargetHit(AActor* Victim)
{
	if (Victim == nullptr || MaxHitsPerTargetPerAttack <= 0)
	{
		return;
	}

	if (FCombatTargetHitRecord* HitRecord = FindHitRecord(Victim))
	{
		++HitRecord->HitCount;
		return;
	}

	FCombatTargetHitRecord& NewHitRecord = TargetHitRecords.AddDefaulted_GetRef();
	NewHitRecord.Target = Victim;
	NewHitRecord.HitCount = 1;
}

FCombatTargetHitRecord* UCombatComponent::FindHitRecord(AActor* Victim)
{
	if (Victim == nullptr)
	{
		return nullptr;
	}

	return TargetHitRecords.FindByPredicate([Victim](const FCombatTargetHitRecord& HitRecord)
	{
		return HitRecord.Target.Get() == Victim;
	});
}

const FCombatTargetHitRecord* UCombatComponent::FindHitRecord(AActor* Victim) const
{
	if (Victim == nullptr)
	{
		return nullptr;
	}

	return TargetHitRecords.FindByPredicate([Victim](const FCombatTargetHitRecord& HitRecord)
	{
		return HitRecord.Target.Get() == Victim;
	});
}

// CombatComponent는 공격자/피격자의 구체 타입에 치우친 전투 처리를 직접 수행하지 않는다.
// 여기서는 히트 결과를 프로젝트 공용 DamageEvent로 정리하고, 피격자에게 필요한 상태만 질의한다.
// 실제 피해 보정, 스태미너 소비, 리액션, 성공 피드백은 해당 피격자/공격자 클래스의 책임으로 둔다.
void UCombatComponent::ApplyDamage(AActor* Victim, const FHitResult& HitResult)
{
	if (Victim == nullptr)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	AController* Instigator = OwnerActor ? OwnerActor->GetInstigatorController() : nullptr;

	// 피격 반응 판단에 필요한 공격 강도와 방향을 TakeDamage 경계로 함께 전달한다.
	FBADamageEvent DamageEvent;
	DamageEvent.DamageReactionType = CurrentDamageReactionType;
	DamageEvent.HitResult = HitResult;
	DamageEvent.LaunchHorizontalSpeed = CurrentLaunchHorizontalSpeed;
	DamageEvent.LaunchVerticalSpeed = CurrentLaunchVerticalSpeed;

	FVector DamageDirection = FVector::ZeroVector;
	if (OwnerActor)
	{
		const FVector OwnerToVictim = Victim->GetActorLocation() - OwnerActor->GetActorLocation();
		const FVector OwnerForward = GetFlatSafeDirection(OwnerActor->GetActorForwardVector());
		const bool bUseForwardForKnockDown =
			CurrentDamageReactionType == EBADamageReactionType::KnockDown
			&& ShouldUseOwnerForwardForKnockDownDirection(
				OwnerActor,
				Victim,
				OwnerToVictim,
				KnockDownDirectionContactTolerance)
			&& !OwnerForward.IsNearlyZero();
		DamageDirection = bUseForwardForKnockDown
			? OwnerForward
			: GetFlatSafeDirection(OwnerToVictim);
	}
	if (DamageDirection.IsNearlyZero())
	{
		DamageDirection = OwnerActor ? GetFlatSafeDirection(OwnerActor->GetActorForwardVector()) : FVector::ZeroVector;
	}

	DamageEvent.DamageDirection = DamageDirection;
	if (ACharacterBase* VictimCharacter = Cast<ACharacterBase>(Victim))
	{
		DamageEvent.bVictimGuarding = VictimCharacter->IsGuardingAgainstDamage(DamageDirection);
		// CharacterBase 질의를 통해 판정하므로 플레이어뿐 아니라 적도 같은 계약을 구현하면 퍼펙트 가드가 가능하다.
		DamageEvent.bVictimPerfectGuard = DamageEvent.bVictimGuarding
			&& VictimCharacter->IsPerfectGuardWindowActive();
	}

	const float AppliedDamage = Victim->TakeDamage(CurrentDamage, DamageEvent, Instigator, OwnerActor);
	OnDamageResolved.Broadcast(Victim, HitResult, AppliedDamage);

	// 충격파 및 왜곡 발생
	SpawnShockwave(HitResult.ImpactPoint, 1.0f);

	// 넉백 처리 - 상대가 Enemy인 경우에만 적용
	// 보스 제외
	if (AMonster* EnemyVictim = Cast<AMonster>(Victim))
	{
		EnemyVictim->ApplyKnockback(OwnerActor, 500.0f);
	}

	if (OnHitDetected.IsBound())
	{
		OnHitDetected.Broadcast(Victim, HitResult);
	}
}
