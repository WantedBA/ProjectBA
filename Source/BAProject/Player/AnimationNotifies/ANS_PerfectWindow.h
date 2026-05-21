#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_PerfectWindow.generated.h"

/**
 * 플레이어 가드 몽타주에서 퍼펙트 가드가 가능한 짧은 구간을 설정하는 노티파이 스테이트.
 */
UCLASS()
class BAPROJECT_API UANS_PerfectWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
