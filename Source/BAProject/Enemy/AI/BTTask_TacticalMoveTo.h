#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_TacticalMoveTo.generated.h"

/**
 * 계산된 TacticalPos로 이동하는 태스크.
 * 이동 중 항상 타겟을 바라보도록 Focus를 조절한다.
 */
UCLASS()
class BAPROJECT_API UBTTask_TacticalMoveTo : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_TacticalMoveTo();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
