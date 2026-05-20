#include "Enemy/AI/BTTask_MoveToRange.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

UBTTask_MoveToRange::UBTTask_MoveToRange()
{
	NodeName = TEXT("Move To Range");
    bNotifyTick = false;
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

    FAIMoveRequest Request;
    Request.SetGoalActor(TargetActor);
    Request.SetAcceptanceRadius(IdealRange);
    Request.SetUsePathfinding(true);
    Request.SetProjectGoalLocation(true);

    // 도달 여부를 테스트할 때 몬스터와 플레이어의 캡슐 컴포넌트 반지름을 계산에서 제외합니다.
    Request.SetReachTestIncludesAgentRadius(false);
    Request.SetReachTestIncludesGoalRadius(false);

    FNavPathSharedPtr OutPath;

    EPathFollowingRequestResult::Type Result = AIController->MoveTo(Request, &OutPath);
    if (Result == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToRange Fail"));
        return EBTNodeResult::Failed;
    }
    else if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToRange AlreadyAtGoal"));
        return EBTNodeResult::Succeeded;
    }
    UPathFollowingComponent* PathFollowingComp = AIController->GetPathFollowingComponent();
    if (PathFollowingComp)
    {
        PathFollowingComp->OnRequestFinished.AddWeakLambda(this, [&OwnerComp](FAIRequestID RequestID, const FPathFollowingResult& Result)
            {
                if (Result.IsSuccess())
                {
                    // 성공적으로 도달했다면 BT에 성공을 알림
                    Cast<UBTTask_MoveToRange>(OwnerComp.GetActiveNode())->FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
                }
                else
                {
                    // 가는 도중 뼈대나 장애물에 막혀 실패했다면 실패를 알림
                    Cast<UBTTask_MoveToRange>(OwnerComp.GetActiveNode())->FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
                }
            });
    }

    UE_LOG(LogTemp, Log, TEXT("MoveToRange InProgress"));
    return EBTNodeResult::InProgress;
}
