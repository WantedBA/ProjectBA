#include "Enemy/AI/BTTask_MoveToRange.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToRange::UBTTask_MoveToRange()
{
	NodeName = TEXT("Move To Range");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveToRange::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (AIController == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
    if (BBComp == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(BBKey::TargetActor));
    if (TargetActor == nullptr)
    {
        return EBTNodeResult::Failed;
    }

    // 패턴별 사거리 우선. 비어있으면(=비-패턴 컨텍스트) 공용 AttackRange로 fallback
    float IdealRange = BBComp->GetValueAsFloat(BBKey::SelectedPatternIdealRange);
    if (IdealRange <= 0.0f)
    {
        IdealRange = BBComp->GetValueAsFloat(BBKey::AttackRange);
    }

    FAIMoveRequest Request;
    Request.SetGoalActor(TargetActor);
    Request.SetAcceptanceRadius(IdealRange);
    Request.SetUsePathfinding(true);
    Request.SetProjectGoalLocation(true);

    FNavPathSharedPtr OutPath;

    EPathFollowingRequestResult::Type Result = AIController->MoveTo(Request, &OutPath);

    if (Result == EPathFollowingRequestResult::Failed)
    {
        return EBTNodeResult::Failed;
    }

    return EBTNodeResult::InProgress;
}

void UBTTask_MoveToRange::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();

    if (AIController == nullptr || BBComp == nullptr)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    APawn* BossPawn = AIController->GetPawn();
    AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(BBKey::TargetActor));

    float IdealRange = BBComp->GetValueAsFloat(BBKey::SelectedPatternIdealRange);
    if (IdealRange <= 0.0f)
    {
        IdealRange = BBComp->GetValueAsFloat(BBKey::AttackRange);
    }

    if (BossPawn == nullptr || TargetActor == nullptr)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    const float Distance = FVector::Dist(BossPawn->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance <= IdealRange)
    {
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    // 안전망: path follow가 끝났는데(=path 종료) 아직 IdealRange 밖이면 path 재요청
    // 타겟이 도망갔거나, AlreadyAtGoal로 시작했거나, NavMesh path 끝점이 IdealRange보다 멀어서 stuck일 때 복구
    UPathFollowingComponent* PathFollow = AIController->GetPathFollowingComponent();
    if (PathFollow && PathFollow->GetStatus() == EPathFollowingStatus::Idle)
    {
        FAIMoveRequest Request;
        Request.SetGoalActor(TargetActor);
        Request.SetAcceptanceRadius(IdealRange);
        Request.SetUsePathfinding(true);
        Request.SetProjectGoalLocation(true);

        AIController->MoveTo(Request);
    }
}
