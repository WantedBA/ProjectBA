#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_GuardWindow.generated.h"

/**
 * 플레이어 가드 몽타주에서 실제 일반 가드 판정이 켜지는 구간을 설정한다.
 *
 * 가드 액션은 Start/Loop/End 섹션 전체에 걸쳐 실행될 수 있지만, 피격을 막는 판정은 Loop 구간에만 있어야 한다.
 * 이 NotifyState를 Loop 전체에 배치하고, 퍼펙트 가드용 ANS_PerfectWindow는 이 구간 안쪽의 더 짧은 구간에 배치한다.
 */
UCLASS()
class BAPROJECT_API UANS_GuardWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
