#include "Enemy/AI/BTTask_MoveToRange.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Enemy/EnemyBase.h"

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

    APawn* BossPawn = AIController->GetPawn();
    if (BossPawn)
    {
        const float CurrentDistance = FVector::Dist(BossPawn->GetActorLocation(), TargetActor->GetActorLocation());
        if (CurrentDistance <= ImmediateAttackRange)
        {
            return EBTNodeResult::Succeeded;
        }
    }

    // 추격 이동 시작 — 보스 상태를 Chase로 올려 MaxWalkSpeed를 100%로 복구한다.
    // (공격 후 Idle(10%)/Alert(40%)로 떨어진 속도가 그대로면 추격이 기어가듯 느려진다)
    if (AEnemyBase* Enemy = Cast<AEnemyBase>(AIController->GetPawn()))
    {
        Enemy->SetState(EEnemyState::Chase);
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

    // 이미 AcceptanceRadius 안에 있으면 이동 없이 바로 성공
    if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        return EBTNodeResult::Succeeded;
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
    // 캡슐 합 + NavMesh 끝점 어긋남에 대한 여유. 정확히 IdealRange로는 도달 못 하는 케이스가 흔함
    const float ArriveBuffer = 50.0f;
    if (Distance <= IdealRange + ArriveBuffer)
    {
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    // NavMesh 최근접 지점에 도달해 이동이 끝난 경우 → 도착으로 처리하고 BT 진행
    // (재요청 시 동일 지점 반복 → 무한 루프 유발이므로 제거)
    UPathFollowingComponent* PathFollow = AIController->GetPathFollowingComponent();
    if (PathFollow && PathFollow->GetStatus() == EPathFollowingStatus::Idle)
    {
        AIController->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
}
