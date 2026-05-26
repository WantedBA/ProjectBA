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
	// 이 거리 이하면 이동 없이 바로 공격 단계로 진행 (보스 패턴 컨텍스트 전용)
	UPROPERTY(EditAnywhere, Category = "MoveToRange")
	float ImmediateAttackRange = 250.0f;

	// IdealRange가 이 값보다 작아도 최소 이 거리까지만 접근 (보스 패턴 컨텍스트 전용)
	UPROPERTY(EditAnywhere, Category = "MoveToRange")
	float MinApproachDistance = 200.0f;

	// 일반 몬스터: AttackRange가 아닌 캡슐 컨택(자/타겟 캡슐 반경 합) + 이 마진까지 접근
	UPROPERTY(EditAnywhere, Category = "MoveToRange")
	float MonsterContactMargin = 10.0f;

private:
	// 몬스터 정지 거리 = Self 캡슐 반경 + Target 캡슐 반경 + MonsterContactMargin
	float ComputeMonsterStopDistance(class APawn* Self, class AActor* Target) const;
};
