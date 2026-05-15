#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SelectPattern_Utility.generated.h"


/**
 * 조건에 따라 가중치값을 주고 Pattern을 선택하는 Task
 */
UCLASS()
class BAPROJECT_API UBTTask_SelectPattern_Utility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SelectPattern_Utility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
