// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_DrinkPotion.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * 
 */
UCLASS()
class BAPROJECT_API UANS_DrinkPotion : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_DrinkPotion();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Potion")
	TObjectPtr<UStaticMesh> PotionMesh;

	UPROPERTY(EditAnywhere, Category = "Potion")
	FName HandSocketName = TEXT("hand_l");

	UPROPERTY(EditAnywhere, Category = "Potion")
	FVector PotionRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Potion")
	FRotator PotionRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Potion")
	FVector PotionRelativeScale = FVector::OneVector;

private:
	void AttachPotionToHand(USkeletalMeshComponent* MeshComp);
	void DetachPotionFromHand(USkeletalMeshComponent* MeshComp);

	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UStaticMeshComponent>> ActivePotionComponents;
};
