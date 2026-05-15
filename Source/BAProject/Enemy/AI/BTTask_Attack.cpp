#include "Enemy/AI/BTTask_Attack.h"
#include "AIController.h"
#include "Enemy/EnemyBase.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn)
	{
		AEnemyBase* Enemy = Cast<AEnemyBase>(ControllingPawn);
		if (Enemy)
		{
			Enemy->Attack();
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
