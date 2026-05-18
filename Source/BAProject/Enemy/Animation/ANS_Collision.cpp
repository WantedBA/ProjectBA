#include "Enemy/Animation/ANS_Collision.h"
#include "Enemy/EnemyBase.h"
#include "Component/CombatComponent.h"

void UANS_Collision::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
	if (Enemy == nullptr)
	{
		return;
	}

	UCombatComponent* CombatComp = Enemy->GetComponentByClass<UCombatComponent>();
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
}

void UANS_Collision::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
	if (Enemy == nullptr)
	{
		return;
	}

	UCombatComponent* CombatComp = Enemy->GetComponentByClass<UCombatComponent>();
	if (CombatComp)
	{
		CombatComp->CheckHitEnd();
	}
}
