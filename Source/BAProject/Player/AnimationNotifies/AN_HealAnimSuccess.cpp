// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AnimationNotifies/AN_HealAnimSuccess.h"

#include "Player/BAPlayerCharacter.h"

void UAN_HealAnimSuccess::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                 const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (ABAPlayerCharacter* PlayerCharacter = Cast<ABAPlayerCharacter>(MeshComp->GetOwner()))
	{
		PlayerCharacter->TryHeal();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AN_HealAnimSuccess notify failed, owner is not ABAPlayerCharacter"));
		return;
	}
}
