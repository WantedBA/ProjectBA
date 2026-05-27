// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AnimationNotifies/ANS_DrinkPotion.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

UANS_DrinkPotion::UANS_DrinkPotion()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PotionMeshAsset(TEXT("/Game/MagicPotion/Meshes/SM_Bottle8.SM_Bottle8"));
	if (PotionMeshAsset.Succeeded())
	{
		PotionMesh = PotionMeshAsset.Object;
	}
}

void UANS_DrinkPotion::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AttachPotionToHand(MeshComp);
}

void UANS_DrinkPotion::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	DetachPotionFromHand(MeshComp);
}

void UANS_DrinkPotion::AttachPotionToHand(USkeletalMeshComponent* MeshComp)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (MeshComp == nullptr || Owner == nullptr || PotionMesh == nullptr)
	{
		return;
	}

	DetachPotionFromHand(MeshComp);

	UStaticMeshComponent* PotionComponent = NewObject<UStaticMeshComponent>(Owner);
	if (PotionComponent == nullptr)
	{
		return;
	}

	PotionComponent->SetStaticMesh(PotionMesh);
	PotionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PotionComponent->SetGenerateOverlapEvents(false);
	PotionComponent->CreationMethod = EComponentCreationMethod::Instance;
	Owner->AddInstanceComponent(PotionComponent);
	PotionComponent->RegisterComponent();
	PotionComponent->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocketName);
	PotionComponent->SetRelativeLocationAndRotation(PotionRelativeLocation, PotionRelativeRotation);
	PotionComponent->SetRelativeScale3D(PotionRelativeScale);

	ActivePotionComponents.Add(TWeakObjectPtr<USkeletalMeshComponent>(MeshComp), PotionComponent);
}

void UANS_DrinkPotion::DetachPotionFromHand(USkeletalMeshComponent* MeshComp)
{
	if (MeshComp == nullptr)
	{
		return;
	}

	TWeakObjectPtr<UStaticMeshComponent> PotionComponent;
	if (!ActivePotionComponents.RemoveAndCopyValue(TWeakObjectPtr<USkeletalMeshComponent>(MeshComp), PotionComponent))
	{
		return;
	}

	if (PotionComponent.IsValid())
	{
		PotionComponent->DestroyComponent();
	}
}
