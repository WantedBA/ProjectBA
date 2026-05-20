#include "Enemy/AI/BTTask_ExecuteBossPattern.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"
#include "Enemy/Boss.h"

UBTTask_ExecuteBossPattern::UBTTask_ExecuteBossPattern()
{
	NodeName = TEXT("ExecuteBossPattern");
	bCreateNodeInstance = true; 
}

EBTNodeResult::Type UBTTask_ExecuteBossPattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	ABoss* Boss = Cast<ABoss>(AIController->GetPawn());
	if (Boss == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;

	int32 PatternTid = BBComp->GetValueAsInt(BBKey::SelectedPatternTid);

	if (PatternTid != 0)
	{
		// 애니메이션 종료 델리게이트 연결
		Boss->OnAttackAnimationFinished.RemoveAll(this);
		TWeakObjectPtr<UBehaviorTreeComponent> WeakOwnerComp(&OwnerComp);

		Boss->OnAttackAnimationFinished.AddLambda([this, WeakOwnerComp](EEnemyState NewState)
		{
			if (WeakOwnerComp.IsValid())
			{
				FinishLatentTask(*WeakOwnerComp, EBTNodeResult::Succeeded);
			}
		});

		Boss->ExecuteBossPattern(PatternTid);

		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::Failed;
}

void UBTTask_ExecuteBossPattern::OnAttackFinishedCallback(EEnemyState NewState)
{
	if (CachedOwnerComp)
	{
		FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	}
}
