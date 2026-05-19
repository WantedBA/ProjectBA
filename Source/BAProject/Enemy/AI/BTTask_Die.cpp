#include "Enemy/AI/BTTask_Die.h"
#include "AIController.h"
#include "Enemy/EnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_Die::UBTTask_Die()
{
	NodeName = TEXT("Die");
}

EBTNodeResult::Type UBTTask_Die::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn)
	{
		// 사실 EnemyBase에서 이미 사망 처리가 되지만, AI를 멈추기 위해 명시적으로 호출하거나 상태를 확인합니다.
		AEnemyBase* Enemy = Cast<AEnemyBase>(ControllingPawn);
		if (Enemy == nullptr)
		{
			return EBTNodeResult::Failed;
		}

		Enemy->OnDeath();
		return EBTNodeResult::Succeeded;
	}

	//Enemy->PlayDeathMontage();
	// Enemy->K2_OnDeadVisuals();
	//Enemy->SpawnDeathFX(); // 이건 Notify에서 진행할 것
	//return InProgress;

	return EBTNodeResult::Failed;
}
