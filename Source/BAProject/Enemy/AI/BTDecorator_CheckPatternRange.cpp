#include "Enemy/AI/BTDecorator_CheckPatternRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"

UBTDecorator_CheckPatternRange::UBTDecorator_CheckPatternRange()
{
	NodeName = TEXT("CheckPatternRange");
}

bool UBTDecorator_CheckPatternRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return false;
	}

	APawn* Boss = AIController->GetPawn();
	if (Boss == nullptr)
	{
		return false;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return false;
	}

	AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(BBKey::TargetActor));
	if (Target == nullptr)
	{
		return false;
	}

	// 블랙보드에서 실시간 거리와 선택된 패턴의 사거리를 가져옴
	float CurrentDistance = BBComp->GetValueAsFloat(BBKey::TargetDistance);
	float PatternRange = BBComp->GetValueAsFloat(BBKey::SelectedPatternIdealRange);
	if (PatternRange <= 0.0f)
	{
		// 패턴 미선택 시 공용 AttackRange로 fallback
		PatternRange = BBComp->GetValueAsFloat(BBKey::AttackRange);
	}

	if (Condition == ERangeCondition::Greater)
	{
		// 추적 조건: 현재 거리가 사거리보다 멀 때
		return CurrentDistance > (PatternRange + AcceptanceRadius);
	}
	else
	{
		// 공격 조건: 현재 거리가 사거리 이내일 때
		return CurrentDistance <= (PatternRange + AcceptanceRadius);
	}
}
