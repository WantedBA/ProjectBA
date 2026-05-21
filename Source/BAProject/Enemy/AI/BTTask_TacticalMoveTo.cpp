#include "Enemy/AI/BTTask_TacticalMoveTo.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"
#include "Navigation/PathFollowingComponent.h"
#include "Enemy/EnemyBase.h"

UBTTask_TacticalMoveTo::UBTTask_TacticalMoveTo()
{
	NodeName = TEXT("TacticalMoveTo");
	bNotifyTick = true; // 이동 중 타겟 주시를 위해 Tick 사용
}

EBTNodeResult::Type UBTTask_TacticalMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BBComponent = OwnerComp.GetBlackboardComponent();
	if(AIController == nullptr || BBComponent == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(AIController->GetPawn());
	if (Enemy == nullptr)
	{
		return EBTNodeResult::Failed;
	}
	
	Enemy->SetState(EEnemyState::Tactical);

	// 타겟 주시 설정
	FVector TargetPos = BBComponent->GetValueAsVector(BBKey::TacticalPos);
	AActor* TargetActor = Cast<AActor>(BBComponent->GetValueAsObject(BBKey::TargetActor));
	if (TargetActor)
	{
		AIController->SetFocus(TargetActor);
	}

	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(TargetPos, 50.0f);
	
	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_TacticalMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController && AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_TacticalMoveTo::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}
