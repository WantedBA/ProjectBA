#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckPatternRange.generated.h"

/**
 * 보스 유틸리티 AI용: 선택된 패턴의 TargetDistance 내에 있는지 체크
 */
UENUM(BlueprintType)
enum class ERangeCondition : uint8
{
	Greater UMETA(DisplayName = "Distance > Range (Chase)"),
	LessEqual UMETA(DisplayName = "Distance <= Range (Attack)")
};
UCLASS()
class BAPROJECT_API UBTDecorator_CheckPatternRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckPatternRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Condition")
	ERangeCondition Condition = ERangeCondition::LessEqual;
	UPROPERTY(EditAnywhere, Category = "Condition")
	float AcceptanceRadius = 50.0f;
};
