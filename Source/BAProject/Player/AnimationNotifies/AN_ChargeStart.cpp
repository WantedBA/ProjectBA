// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AnimationNotifies/AN_ChargeStart.h"

#include "Player/BAPlayerCharacter.h"

void UAN_ChargeStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                             const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (ABAPlayerCharacter* Character = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
	{
		Character->ChargeLoopStart(MeshComp, Animation);
	}
}
