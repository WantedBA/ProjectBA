#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckDistance.generated.h"

UENUM(BlueprintType)
enum class EDistanceCondition : uint8
{
    Distance_Greater UMETA(DisplayName = "Distance > Range (Chase)"),
    Distance_LessEqual UMETA(DisplayName = "Distance <= Range (Attack)")
};

/**
 * 조건문 용도, TargetActor와의 거리가 AttackRange 기준 조건에 맞는지 체크하는 데코레이터
 */
UCLASS()
class BAPROJECT_API UBTDecorator_CheckDistance : public UBTDecorator
{
	GENERATED_BODY()

public:
    UBTDecorator_CheckDistance();

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    UPROPERTY(EditAnywhere, Category = "Condition")
    EDistanceCondition Condition;
};
