#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTargetContext.generated.h"

/**
 * 타겟 감지 및 거리/각도 등 Utility AI 계산에 필요한 맥락 정보를 업데이트하는 서비스
 */
UCLASS()
class BAPROJECT_API UBTService_UpdateTargetContext : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateTargetContext();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
