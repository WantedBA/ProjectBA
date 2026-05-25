#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToRange.generated.h"

UCLASS()
class BAPROJECT_API UBTTask_MoveToRange : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToRange();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	// 이 거리 이하면 이동 없이 바로 공격 단계로 진행
	UPROPERTY(EditAnywhere, Category = "MoveToRange")
	float ImmediateAttackRange = 250.0f;
};
