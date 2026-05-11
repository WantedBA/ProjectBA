#include "Enemy/AI/BTDecorator_CheckDistance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"

UBTDecorator_CheckDistance::UBTDecorator_CheckDistance()
{
    NodeName = TEXT("CheckDistance");
}

bool UBTDecorator_CheckDistance::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

    APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (ControllingPawn == nullptr)
    {
        return false;
    }

    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
    if (BBComp == nullptr)
    {
        return false;
    }

    AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(BBKey::TargetActor));
    if (TargetActor == nullptr)
    {
        return false;
    }

    float AttackRange = BBComp->GetValueAsFloat(BBKey::AttackRange);
    float Distance = FVector::Dist(ControllingPawn->GetActorLocation(), TargetActor->GetActorLocation());

    if (Condition == EDistanceCondition::Distance_Greater)
    {
        return Distance > AttackRange;
    }
    else
    {
        return Distance <= AttackRange;
    }
}
