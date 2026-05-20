#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToRange.generated.h"

UCLASS()
class BAPROJECT_API UBTTask_MoveToRange : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_MoveToRange();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
