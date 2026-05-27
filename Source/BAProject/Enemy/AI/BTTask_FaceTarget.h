#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FaceTarget.generated.h"

/**
 * 보스를 타겟(플레이어) 쪽으로 회전 몽타주를 재생해 정렬시키는 태스크.
 * Combat 시퀀스 선두에 두어, 패턴 선택 전 보스를 플레이어 정면으로 맞춘다.
 *
 * - 회전이 필요하면(정면 기준 90도 초과) 턴 몽타주 재생 → 종료까지 대기 후 Succeeded
 * - 회전 불필요(90도 이내)면 즉시 Succeeded → Combat 시퀀스가 SelectPattern으로 진행
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
