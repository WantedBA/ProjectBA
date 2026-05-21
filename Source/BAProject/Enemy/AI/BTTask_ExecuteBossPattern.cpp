#include "Enemy/AI/BTTask_ExecuteBossPattern.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"
#include "Enemy/Boss.h"

UBTTask_ExecuteBossPattern::UBTTask_ExecuteBossPattern()
{
	NodeName = TEXT("ExecuteBossPattern");
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ExecuteBossPattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	ABoss* Boss = Cast<ABoss>(AIController->GetPawn());
	if (Boss == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	const int32 PatternTid = BBComp->GetValueAsInt(BBKey::SelectedPatternTid);
	UE_LOG(LogTemp, Warning, TEXT("[BTTask_ExecuteBossPattern] ExecuteTask Tid=%d"), PatternTid);

	if (PatternTid == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_ExecuteBossPattern] FAILED (PatternTid=0)"));
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;
	CachedBoss = Boss;

	// 멤버 함수로 바인딩해야 RemoveAll(this)가 실제로 동작한다.
	// AddLambda로 붙인 델리게이트는 바인딩 오브젝트가 없어 RemoveAll로 제거되지 않으며,
	// 패턴을 쓸 때마다 콜백이 누적되는 버그가 된다.
	Boss->OnAttackAnimationFinished.RemoveAll(this);
	Boss->OnAttackAnimationFinished.AddUObject(this, &UBTTask_ExecuteBossPattern::OnAttackFinishedCallback);

	Boss->ExecuteBossPattern(PatternTid);

	UE_LOG(LogTemp, Warning, TEXT("[BTTask_ExecuteBossPattern] ExecuteBossPattern returned, awaiting Notify"));
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_ExecuteBossPattern::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 피격 인터럽트 등으로 트리가 이 태스크를 중단시킬 때 델리게이트 누수를 막는다.
	CleanupDelegate();
	return EBTNodeResult::Aborted;
}

void UBTTask_ExecuteBossPattern::OnAttackFinishedCallback(EEnemyState NewState)
{
	UE_LOG(LogTemp, Warning, TEXT("[BTTask_ExecuteBossPattern] OnAttackFinishedCallback, state=%d"), (int32)NewState);

	CleanupDelegate();

	if (CachedOwnerComp.IsValid())
	{
		FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_ExecuteBossPattern::CleanupDelegate()
{
	if (CachedBoss.IsValid())
	{
		CachedBoss->OnAttackAnimationFinished.RemoveAll(this);
	}
}
