#include "Enemy/AI/BTTask_FindPatrolPos.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "Constants/BAProjectConstant.h"

UBTTask_FindPatrolPos::UBTTask_FindPatrolPos()
{
    NodeName = TEXT("FindPatrolPos");
    PatrolRadius = 500.f; // 기본 순찰 범위
}

EBTNodeResult::Type UBTTask_FindPatrolPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

    APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (ControllingPawn == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(ControllingPawn->GetWorld());
    if (NavSystem == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BBComponent = OwnerComp.GetBlackboardComponent();
    if (BBComponent == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    // HomePos(스폰 위치)를 기준으로 랜덤 위치 탐색
    FVector HomePos = BBComponent->GetValueAsVector(BBKey::HomePos);
    FNavLocation NextPatrolPos;

    if (NavSystem->GetRandomPointInNavigableRadius(HomePos, PatrolRadius, NextPatrolPos))
    {
        BBComponent->SetValueAsVector(BBKey::PatrolPos, NextPatrolPos.Location);
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}
