#include "Enemy/Animation/ANS_PerfectWindow.h"
#include "Enemy/EnemyBase.h"
#include "Component/CombatComponent.h"

void UANS_PerfectWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner()))
		{
			UCombatComponent* Combat = Enemy->FindComponentByClass<UCombatComponent>();
			if (Combat)
			{
				Combat->SetPerfectWindowActive(true, SuccessTimeDilation, SuccessDuration);
			}
		}
	}
}

void UANS_PerfectWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner()))
		{
			UCombatComponent* Combat = Enemy->FindComponentByClass<UCombatComponent>();
			if (Combat)
			{
				Combat->SetPerfectWindowActive(false);
			}
		}
	}
}
