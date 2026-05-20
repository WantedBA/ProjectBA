#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_PerfectWindow.generated.h"

/**
 * 몬스터의 공격 중 '퍼펙트 가드/회피'가 가능한 구간을 설정하는 노티파이 스테이트.
 */
UCLASS()
class BAPROJECT_API UANS_PerfectWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	// 성공 시 시간 팽창 배율
	UPROPERTY(EditAnywhere, Category = "PerfectWindow")
	float SuccessTimeDilation = 0.8f;

	// 성공 시 슬로우 모션 지속 시간
	UPROPERTY(EditAnywhere, Category = "PerfectWindow")
	float SuccessDuration = 0.1f;
};
