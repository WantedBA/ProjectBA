#include "Enemy/Animation/ANS_SuperArmor.h"
#include "Enemy/EnemyBase.h"

void UANS_SuperArmor::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
	if (Enemy)
	{
		// 슈퍼아머 상태 활성화
		Enemy->SetSuperArmor(true);
	}
}

void UANS_SuperArmor::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
	if (Enemy)
	{
		// 슈퍼아머 상태 비활성화
		Enemy->SetSuperArmor(false);
	}
}
