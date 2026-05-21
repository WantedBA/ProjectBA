#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Enemy/Boss.h"
#include "BTTask_ExecuteBossPattern.generated.h"

/**
 * 보스 유틸리티 AI에서 선택된 패턴을 실제 실행하는 태스크
 */
UCLASS()
class BAPROJECT_API UBTTask_ExecuteBossPattern : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ExecuteBossPattern();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	void OnAttackFinishedCallback(EEnemyState NewState);

private:
	// 보스 OnAttackAnimationFinished 델리게이트에서 이 태스크 바인딩을 해제한다.
	void CleanupDelegate();

	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
	TWeakObjectPtr<ABoss> CachedBoss;
};
