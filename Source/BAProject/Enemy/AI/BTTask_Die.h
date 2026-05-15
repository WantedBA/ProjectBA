#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Die.generated.h"

UCLASS()
class BAPROJECT_API UBTTask_Die : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Die();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
