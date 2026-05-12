#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_DetectTarget.generated.h"

/*
* 값 감지용도: 순찰루트를 돌면서 주기적으로 Player를 찾는 로직
*/

UCLASS()
class BAPROJECT_API UBTService_DetectTarget : public UBTService
{
	GENERATED_BODY()

public:
    UBTService_DetectTarget();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category = "Target")
    float DetectRange;
};
