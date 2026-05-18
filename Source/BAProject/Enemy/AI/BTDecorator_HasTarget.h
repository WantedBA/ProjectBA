#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasTarget.generated.h"

UCLASS()
class BAPROJECT_API UBTDecorator_HasTarget : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HasTarget();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
