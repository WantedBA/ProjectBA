#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsDead.generated.h"

UCLASS()
class BAPROJECT_API UBTDecorator_IsDead : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IsDead();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
