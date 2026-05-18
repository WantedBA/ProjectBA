#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "Enemy/EnemyBase.h"
#include "BTDecorator_CheckState.generated.h"

UCLASS()
class BAPROJECT_API UBTDecorator_CheckState : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckState();
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

private:
	UPROPERTY(EditAnywhere, Category = "Condition")
	EEnemyState CheckState;
};
