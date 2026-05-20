#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Enemy/EnemyBase.h"
#include "BTTask_Attack.generated.h"

UCLASS()
class BAPROJECT_API UBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Attack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	void OnAttackFinishedCallback(EEnemyState NewState);

private:
	TObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
};
