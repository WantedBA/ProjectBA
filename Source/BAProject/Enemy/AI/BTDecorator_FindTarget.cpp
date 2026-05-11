#include "Enemy/AI/BTDecorator_FindTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"

UBTDecorator_FindTarget::UBTDecorator_FindTarget()
{
    NodeName = TEXT("FindAndSetTarget");
}

bool UBTDecorator_FindTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

    UBlackboardComponent* BBComponent = OwnerComp.GetBlackboardComponent();
    if (BBComponent == nullptr)
    {
        return false;
    }

    UObject* Target = BBComponent->GetValueAsObject(BBKey::TargetActor);

    return Target != nullptr;
}
