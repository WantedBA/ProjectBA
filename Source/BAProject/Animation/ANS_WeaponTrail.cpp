#include "Animation/ANS_WeaponTrail.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

void UANS_WeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (TrailSystem == nullptr || MeshComp == nullptr) return;

	TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TrailSystem, MeshComp, AttachSocket,
		FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset, true
	);
}

void UANS_WeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (TrailComponent)
	{
		TrailComponent->Deactivate();
		TrailComponent = nullptr;
	}
}
