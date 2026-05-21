#include "Player/AnimationNotifies/ANS_PerfectWindow.h"

#include "Player/BAPlayerCharacter.h"

void UANS_PerfectWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
		{
			Player->SetPerfectGuardWindowActive(true);
		}
	}
}

void UANS_PerfectWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
		{
			Player->SetPerfectGuardWindowActive(false);
		}
	}
}
