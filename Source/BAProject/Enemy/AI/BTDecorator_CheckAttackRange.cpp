#include "Enemy/AI/BTDecorator_CheckAttackRange.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"

UBTDecorator_CheckAttackRange::UBTDecorator_CheckAttackRange()
{
    NodeName = TEXT("CheckAttackRange");
}

bool UBTDecorator_CheckAttackRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (ControllingPawn == nullptr)
    {
        return false;
    }

    AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BBKey::TargetActor));
    if (Target == nullptr)
    {
        return false;
    }

    float Distance = FVector::Dist(ControllingPawn->GetActorLocation(), Target->GetActorLocation());
    float AttackRange = OwnerComp.GetBlackboardComponent()->GetValueAsFloat(BBKey::AttackRange);

    if (Condition == EDistanceCondition::Distance_Greater)
    {
        return Distance > AttackRange;
    }
    else
    {
        return Distance <= AttackRange;
    }
}
