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
		CombatComp->SetWeaponCollisionActive(true);
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
		CombatComp->SetWeaponCollisionActive(false);
	}
}
