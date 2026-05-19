#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasPatrolPath.generated.h"

UCLASS()
class BAPROJECT_API UBTDecorator_HasPatrolPath : public UBTDecorator
{
	GENERATED_BODY()

public:
    UBTDecorator_HasPatrolPath();

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
