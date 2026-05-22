#include "Enemy/Animation/ANS_BossAreaAttack.h"
#include "Character/CharacterBase.h"
#include "Component/CombatComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

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

	FVector Origin = Owner->GetActorLocation();
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

#if ENABLE_DRAW_DEBUG
	if (bShowDebugRadius)
	{
		DrawDebugSphere(Owner->GetWorld(), Origin, Radius, 32, FColor::Orange, false, TotalDuration, 0, 2.0f);
	}
#endif

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
			Origin,
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
