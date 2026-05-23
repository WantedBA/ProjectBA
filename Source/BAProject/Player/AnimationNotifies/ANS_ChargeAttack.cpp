// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AnimationNotifies/ANS_ChargeAttack.h"

#include "Player/BAPlayerCharacter.h"

void UANS_ChargeAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (ABAPlayerCharacter* Character = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
	{
		Character->ChargeAttackStart();
	}
}

void UANS_ChargeAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (ABAPlayerCharacter* Character = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
	{
		Character->ChargeLoopStart(MeshComp, Animation);
	}
}
