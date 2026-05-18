#include "Enemy/AI/BTTask_MoveToRange.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

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

    float IdealRange = BBComp->GetValueAsFloat(BBKey::AttackRange);

    // IdealRange 내로 유동적 이동
    //EPathFollowingRequestResult::Type RequestResult = AIController->MoveToActor(TargetActor, IdealRange, true, true, true, 0, true);
    //switch (RequestResult)
    //{
    //case EPathFollowingRequestResult::Type::RequestSuccessful:
    //    return EBTNodeResult::InProgress;

    //case EPathFollowingRequestResult::Type::AlreadyAtGoal:
    //    return EBTNodeResult::Succeeded;

    //case EPathFollowingRequestResult::Type::Failed:
    //    return EBTNodeResult::Failed;
    //}

    //return EBTNodeResult::Failed;
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
    float IdealRange = BBComp->GetValueAsFloat(BBKey::AttackRange);

    if (BossPawn == nullptr || TargetActor == nullptr)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    float Distance = FVector::Dist(BossPawn->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance <= IdealRange)
    {
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}
