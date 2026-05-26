#include "Enemy/Animation/ANS_Collision.h"
#include "Enemy/EnemyBase.h"
#include "Component/CombatComponent.h"
#include "Component/PlayerWeaponVFX.h"

void UANS_Collision::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	ACharacterBase* CharacterBase = Cast<ACharacterBase>(MeshComp->GetOwner());
	if (CharacterBase == nullptr)
	{
		return;
	}

	UCombatComponent* CombatComp = CharacterBase->GetComponentByClass<UCombatComponent>();
	if (CombatComp)
	{
		if (bUseDefaultData)
		{
			CombatComp->CheckHitStartDefault();
		}
		else
		{
			// ANS에서 직접 지정한 소켓들과 반경 사용 (데미지는 현재 컴포넌트에 설정된 값 유지)
			CombatComp->CheckHitStart(HitRadius, -1.0f, StartSocket, EndSocket);
		}
	}

	// 충돌 시간동안 트레일 설정
	if (UPlayerWeaponVFX* WeaponVfx = CharacterBase->GetComponentByClass<UPlayerWeaponVFX>())
	{
		WeaponVfx->ActivateTrailNiagara();
	}
}

void UANS_Collision::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	ACharacterBase* CharacterBase = Cast<ACharacterBase>(MeshComp->GetOwner());
	if (CharacterBase == nullptr)
	{
		return;
	}

	UCombatComponent* CombatComp = CharacterBase->GetComponentByClass<UCombatComponent>();
	if (CombatComp)
	{
		CombatComp->CheckHitEnd();
	}
	
	// 충돌 시간동안 트레일 설정
	if (UPlayerWeaponVFX* WeaponVfx = CharacterBase->GetComponentByClass<UPlayerWeaponVFX>())
	{
		WeaponVfx->DeactivateTrailNiagara();
	}
}
