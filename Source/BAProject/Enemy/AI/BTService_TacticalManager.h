#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_TacticalManager.generated.h"

/**
 * 개체별 전술적 이동(Strafing, Spacing)을 관리하는 서비스.
 * 공격 쿨다운 중에 플레이어 주변을 서성이며 위압감을 조성한다.
 */
UCLASS()
class BAPROJECT_API UBTService_TacticalManager : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_TacticalManager();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	// 군무 방지를 위한 개체별 내부 변수
	float NextDirectionChangeTime = 0.0f;
	float PersonalSpacingOffset = 0.0f;
};
