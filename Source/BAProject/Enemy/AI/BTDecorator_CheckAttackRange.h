#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckAttackRange.generated.h"

UENUM(BlueprintType)
enum class EDistanceCondition : uint8
{
    Distance_Greater UMETA(DisplayName = "Distance > Range (Chase)"),
    Distance_LessEqual UMETA(DisplayName = "Distance <= Range (Attack)")
};

/**
 * 일반 몬스터의 기본 AttackRange 기준 조건에 맞는지 체크하는 데코레이터
 */
UCLASS()
class BAPROJECT_API UBTDecorator_CheckAttackRange : public UBTDecorator
{
	GENERATED_BODY()

public:
    UBTDecorator_CheckAttackRange();

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    UPROPERTY(EditAnywhere, Category = "Condition")
    EDistanceCondition Condition;
};
