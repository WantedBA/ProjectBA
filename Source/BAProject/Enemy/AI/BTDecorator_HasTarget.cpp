#include "Enemy/AI/BTDecorator_HasTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"

UBTDecorator_HasTarget::UBTDecorator_HasTarget()
{
	NodeName = TEXT("HasTarget");
}

bool UBTDecorator_HasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return false;
	}

	AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(BBKey::TargetActor));
	if(Target == nullptr)
	{
		return false;
	}

	return true;
}
