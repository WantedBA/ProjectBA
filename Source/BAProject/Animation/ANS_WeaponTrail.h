#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_WeaponTrail.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class BAPROJECT_API UANS_WeaponTrail : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "WeaponTrail")
	TObjectPtr<UNiagaraSystem> TrailSystem;

	// 트레일을 붙일 소켓 (미지정 시 루트에 부착)
	UPROPERTY(EditAnywhere, Category = "WeaponTrail")
	FName AttachSocket = NAME_None;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailComponent;
};
