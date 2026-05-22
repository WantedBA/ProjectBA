// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AnimationNotifies/ANS_PlayerChargeAttack.h"

#include "Player/BAPlayerCharacter.h"

void UANS_PlayerChargeAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                          float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
 	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (ABAPlayerCharacter* PlayerCharacter = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
	{
		PlayerCharacter->ChargeStart();
	}
	
}

void UANS_PlayerChargeAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}
