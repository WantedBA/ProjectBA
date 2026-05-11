#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolPos.generated.h"

/**
 * 스폰 위치(HomePos) 주변의 랜덤한 순찰 위치를 찾는 Task
 */
UCLASS()
class BAPROJECT_API UBTTask_FindPatrolPos : public UBTTaskNode
{
	GENERATED_BODY()

public:
    UBTTask_FindPatrolPos();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
    UPROPERTY(EditAnywhere, Category = "Patrol")
    float PatrolRadius;
};
