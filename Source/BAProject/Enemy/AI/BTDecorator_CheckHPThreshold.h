#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckHPThreshold.generated.h"

/**
 * 특정 HP 임계치 조건에 맞는지 체크하는 범용 데코레이터
 */
UCLASS()
class BAPROJECT_API UBTDecorator_CheckHPThreshold : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckHPThreshold();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	float HPThreshold = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bLessEqual = true;
};
