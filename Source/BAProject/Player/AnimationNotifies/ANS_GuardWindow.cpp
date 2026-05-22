#include "Player/AnimationNotifies/ANS_GuardWindow.h"

#include "Player/BAPlayerCharacter.h"

void UANS_GuardWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
		{
			Player->SetGuardWindowActive(true);
		}
	}
}

void UANS_GuardWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
		{
			Player->SetGuardWindowActive(false);
		}
	}
}
