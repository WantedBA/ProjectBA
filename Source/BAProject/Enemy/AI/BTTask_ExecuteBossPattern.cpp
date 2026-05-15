#include "Enemy/AI/BTTask_ExecuteBossPattern.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Enemy/Boss.h"
#include "Constants/BAProjectConstant.h"

UBTTask_ExecuteBossPattern::UBTTask_ExecuteBossPattern()
{
	NodeName = TEXT("ExecuteBossPattern");
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

	int32 PatternIndex = BBComp->GetValueAsInt(BBKey::SelectedPatternIndex);

	const auto& Patterns = Boss->GetBossPatterns();
	if (Patterns.IsValidIndex(PatternIndex))
	{
		int32 SelectedTid = Patterns[PatternIndex].Tid;
		float CoolTime = Patterns[PatternIndex].CoolTime;

		Boss->ExecuteBossPattern(SelectedTid);
		Boss->StartPatternCooldown(SelectedTid, CoolTime);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
