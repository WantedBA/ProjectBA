#include "Enemy/AI/BTTask_ExecuteBossPattern.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"

UBTTask_ExecuteBossPattern::UBTTask_ExecuteBossPattern()
{
	NodeName = TEXT("ExecuteBossPattern");

	bCreateNodeInstance = true; // Node Instance를 Share, 여러 AI가 같은 Task를 사용 할 수 있어서
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

	int32 PatternIndex = BBComp->GetValueAsInt(BBKey::SelectedPatternIndex);

	const auto& Patterns = Boss->GetBossPatterns();
	if (Patterns.IsValidIndex(PatternIndex))
	{
		//Boss->OnAttackAnimationFinished.RemoveAll(this);
		//Boss->OnAttackAnimationFinished.AddUObject(this, &UBTTask_ExecuteBossPattern::OnAttackFinishedCallback);

		//int32 SelectedTid = Patterns[PatternIndex].Tid;
		//float CoolTime = Patterns[PatternIndex].CoolTime;

		//Boss->ExecuteBossPattern(SelectedTid);
		//Boss->StartPatternCooldown(SelectedTid, CoolTime);
		//return EBTNodeResult::InProgress;
		Boss->OnAttackAnimationFinished.RemoveAll(this);
		TWeakObjectPtr<UBehaviorTreeComponent> WeakOwnerComp(&OwnerComp);
		Boss->OnAttackAnimationFinished.AddLambda([this, WeakOwnerComp](EEnemyState NewState)
			{
				if (WeakOwnerComp.IsValid())
				{
					FinishLatentTask(*WeakOwnerComp, EBTNodeResult::Succeeded);
				}
			});

		int32 SelectedTid = Patterns[PatternIndex].Tid;
		float CoolTime = Patterns[PatternIndex].CoolTime;

		Boss->ExecuteBossPattern(SelectedTid);
		Boss->StartPatternCooldown(SelectedTid, CoolTime);

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
