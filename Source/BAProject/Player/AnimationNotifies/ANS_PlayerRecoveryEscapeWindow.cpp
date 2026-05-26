#include "Player/AnimationNotifies/ANS_PlayerRecoveryEscapeWindow.h"

#include "Player/BAPlayerCharacter.h"

void UANS_PlayerRecoveryEscapeWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp)
	{
		if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
		{
			Player->OpenRecoveryEscapeWindow(Settings);
		}
	}
}

void UANS_PlayerRecoveryEscapeWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
		{
			Player->CloseRecoveryEscapeWindow(Settings);
		}
	}
}
