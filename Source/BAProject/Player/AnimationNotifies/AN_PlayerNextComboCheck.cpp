// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AnimationNotifies/AN_PlayerNextComboCheck.h"

#include "Player/BAPlayerCharacter.h"

void UAN_PlayerNextComboCheck::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation);
	
	if (MeshComp == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("MeshComp is null in UAN_PlayerNextComboCheck::Notify"));
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (Owner == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner is null in UAN_PlayerNextComboCheck::Notify"));
		return;
	}

	if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(Owner); IsValid(Player))
	{
		Player->OnNextComboCheck();
	}
}
