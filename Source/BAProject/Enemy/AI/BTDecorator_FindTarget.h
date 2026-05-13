#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_FindTarget.generated.h"

/*
* 조건문 용도, BlackBoard에 Target을 체크하여 Battle, Patrol 분기함
*/

UCLASS()
class BAPROJECT_API UBTDecorator_FindTarget : public UBTDecorator
{
	GENERATED_BODY()

public:
    UBTDecorator_FindTarget();

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
