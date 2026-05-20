#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FaceTarget.generated.h"

/**
 * 보스를 타겟(플레이어) 쪽으로 회전 몽타주를 재생해 정렬시키는 태스크.
 * 패턴 선택 실패(측면/후방이라 정면 패턴이 막힘) 시 폴백으로 사용.
 *
 * - 회전이 필요하면 보스의 턴 몽타주 재생 → 몽타주 종료까지 대기 후 Succeeded
 * - 이미 정면(±45° 이내)이면 회전 불필요 → Failed (Selector가 Idle 등 다음 분기로 진행)
 */
UCLASS()
class BAPROJECT_API UBTTask_FaceTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FaceTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
