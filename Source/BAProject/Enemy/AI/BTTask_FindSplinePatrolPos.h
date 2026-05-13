#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindSplinePatrolPos.generated.h"

/**
 * Spline을 따라 다음 순찰 지점을 찾는 Task
 */
UCLASS()
class BAPROJECT_API UBTTask_FindSplinePatrolPos : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindSplinePatrolPos();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
