#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_SetMontagePlayRate.generated.h"

UCLASS()
class BAPROJECT_API UANS_SetMontagePlayRate : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	// 이 구간에서 적용할 재생 속도
	UPROPERTY(EditAnywhere, Category = "PlayRate", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;

	// 구간 종료 시 복원할 재생 속도
	UPROPERTY(EditAnywhere, Category = "PlayRate", meta = (ClampMin = "0.01"))
	float RestorePlayRate = 1.0f;
};
