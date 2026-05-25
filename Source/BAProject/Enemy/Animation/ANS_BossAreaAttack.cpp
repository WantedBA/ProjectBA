#include "Enemy/Animation/ANS_BossAreaAttack.h"
#include "Character/CharacterBase.h"
#include "Component/CombatComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

FVector UANS_BossAreaAttack::GetOrigin(AActor* Owner) const
{
	if (OriginComponentTag != NAME_None)
	{
		TArray<UActorComponent*> Comps = Owner->GetComponentsByTag(USceneComponent::StaticClass(), OriginComponentTag);
		if (Comps.Num() > 0)
		{
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comps[0]))
			{
				return SceneComp->GetComponentLocation();
			}
		}
	}
	return Owner->GetActorLocation();
}

void UANS_BossAreaAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (Owner == nullptr)
	{
		return;
	}

	// 데미지는 보스 CombatComponent에 패턴 로드 시 설정된 엑셀값 사용
	float Damage = 0.0f;
	if (UCombatComponent* CombatComp = Owner->FindComponentByClass<UCombatComponent>())
	{
		Damage = CombatComp->GetCurrentDamage();
	}

	FVector Origin = GetOrigin(Owner);
	FVector VFXOrigin = Origin + Owner->GetActorRotation().RotateVector(VFXSpawnOffset);
	AController* InstigatorController = Owner->GetInstigatorController();

	TArray<FHitResult> OutHits;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner);

	UKismetSystemLibrary::SphereTraceMulti(
		Owner,
		Origin,
		Origin,
		Radius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		OutHits,
		true
	);

	TArray<AActor*> HitActors;
	for (const FHitResult& Hit : OutHits)
	{
		AActor* Victim = Hit.GetActor();
		if (Victim == nullptr || HitActors.Contains(Victim))
		{
			continue;
		}

		// 캡슐 표면이 아닌 Actor 중심 기준으로 반경 재검증 (SphereTrace는 콜리전 표면 기준이라 캡슐 반경만큼 범위가 넓어짐)
		const float DistSq = FVector::DistSquared(Victim->GetActorLocation(), Origin);
		if (DistSq > Radius * Radius)
		{
			continue;
		}

		HitActors.Add(Victim);

		FVector DamageDirection = (Victim->GetActorLocation() - Origin).GetSafeNormal();

		FBADamageEvent DamageEvent;
		DamageEvent.DamageReactionType = DamageReactionType;
		DamageEvent.HitResult = Hit;
		DamageEvent.DamageDirection = DamageDirection;

		if (ACharacterBase* VictimCharacter = Cast<ACharacterBase>(Victim))
		{
			DamageEvent.bVictimGuarding = VictimCharacter->IsGuardingAgainstDamage(DamageDirection);
			DamageEvent.bVictimPerfectGuard = DamageEvent.bVictimGuarding
				&& VictimCharacter->IsPerfectGuardWindowActive();
		}

		Victim->TakeDamage(Damage, DamageEvent, InstigatorController, Owner);
	}

	// VFX 스폰. 스케일을 Radius에 맞게 자동 조정한다.
	ActiveVFXList.Reset();
	const float VFXScale = (VFXBaseRadius > 0.0f) ? (Radius / VFXBaseRadius) : 1.0f;

	for (UNiagaraSystem* VFX : AreaVFXList)
	{
		if (VFX == nullptr)
		{
			continue;
		}

		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Owner,
			VFX,
			VFXOrigin,
			Owner->GetActorRotation(),
			FVector(VFXScale),
			true,  // bAutoDestroy: NotifyEnd 이후 파티클이 자연 소멸하면 자동 제거
			true,
			ENCPoolMethod::None
		);

		if (NiagaraComp)
		{
			ActiveVFXList.Add(NiagaraComp);
		}
	}
}

void UANS_BossAreaAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

#if ENABLE_DRAW_DEBUG
	if (bShowDebugRadius)
	{
		AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		if (Owner)
		{
			DrawDebugSphere(Owner->GetWorld(), GetOrigin(Owner), Radius, 24, FColor::Orange, false, -1.f, 0, 2.f);
		}
	}
#endif
}

void UANS_BossAreaAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	for (UNiagaraComponent* NiagaraComp : ActiveVFXList)
	{
		if (NiagaraComp)
		{
			// 신규 파티클 생성을 중단하고 기존 파티클은 수명대로 자연 소멸
			NiagaraComp->Deactivate();
		}
	}
	ActiveVFXList.Reset();
}
