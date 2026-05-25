#include "Animation/ANS_SetMontagePlayRate.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

void UANS_SetMontagePlayRate::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr)
	{
		return;
	}

	UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (Montage == nullptr)
	{
		return;
	}

	AnimInstance->Montage_SetPlayRate(Montage, PlayRate);
}

void UANS_SetMontagePlayRate::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr)
	{
		return;
	}

	UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (Montage == nullptr)
	{
		return;
	}

	AnimInstance->Montage_SetPlayRate(Montage, RestorePlayRate);
}
