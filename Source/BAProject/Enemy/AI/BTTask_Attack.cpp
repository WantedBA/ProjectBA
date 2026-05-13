#include "Enemy/AI/BTTask_Attack.h"
#include "AIController.h"
#include "Enemy/EnemyBase.h"
#include "GameFramework/Character.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyBase* Enemy = Cast<AEnemyBase>(OwnerComp.GetAIOwner()->GetPawn());
	if (Enemy == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Execute Attack Task - Calling Attack()"), *Enemy->GetName());
	
	Enemy->Attack();
	
	bIsAttacking = true;
	
	return EBTNodeResult::Succeeded;
}

void UBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	
	// 애니메이션 종료 체크 등을 여기서 수행 가능
}
