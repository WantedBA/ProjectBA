#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Enemy/EnemyBase.h"
#include "BTTask_PlayMontageWithState.generated.h"

/**
 * 특정 상태(Hit, Stagger 등)의 몽타주를 재생하고 완료될 때까지 대기하는 태스크.
 * 완료 후 상태를 Idle로 복구한다.
 */
UCLASS()
class BAPROJECT_API UBTTask_PlayMontageWithState : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PlayMontageWithState();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	void OnMontageFinished(UAnimMontage* Montage, bool bInterrupted, TWeakObjectPtr<UBehaviorTreeComponent> OwnerCompPtr);

private:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	EEnemyState TargetState;
};
