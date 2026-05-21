#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_PerfectWindow.generated.h"

/**
 * 플레이어 가드 몽타주에서 퍼펙트 가드가 가능한 짧은 구간을 설정하는 노티파이 스테이트.
 *
 * 현재 퍼펙트 가드는 “가드 액션 데이터의 숫자”보다 “가드 몽타주 안의 아주 짧은 타이밍”에 더 강하게 묶여 있다.
 * 그래서 구간을 데이터 테이블 row로 분리하지 않고 NotifyState로 유지한다. 몽타주 타임라인에서 시작/끝 프레임을
 * 직접 보면서 조정할 수 있어 애니메이션과 판정이 어긋나는 문제를 줄이기 쉽기 때문이다.
 *
 * 나중에 무기, 스킬, 캐릭터별 퍼펙트 가드 윈도우를 대량으로 관리하거나 밸런싱해야 하면 이 책임은 data row로
 * 옮기는 것이 맞다. 그때는 NotifyState가 직접 판정 시간을 소유하기보다 window id 또는 marker만 제공하고,
 * 실제 윈도우 길이와 정책은 별도의 퍼펙트 가드 윈도우 데이터에서 읽도록 바꾼다.
 */
UCLASS()
class BAPROJECT_API UANS_PerfectWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
