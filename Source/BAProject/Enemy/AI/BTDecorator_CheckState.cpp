#include "Enemy/AI/BTDecorator_CheckState.h"
#include "AIController.h"

UBTDecorator_CheckState::UBTDecorator_CheckState()
{
    NodeName = TEXT("CheckState");
}

bool UBTDecorator_CheckState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    AEnemyBase* Enemy = Cast<AEnemyBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (Enemy == nullptr)
    {
        return false;
    }

    return Enemy->GetCurrentState() == CheckState;
}
