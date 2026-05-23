#include "Enemy/AI/BTTask_Attack.h"
#include "AIController.h"
#include "Enemy/EnemyBase.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn)
	{
		AEnemyBase* Enemy = Cast<AEnemyBase>(ControllingPawn);
		if (Enemy)
		{
			CachedOwnerComp = &OwnerComp;

			Enemy->OnAttackAnimationFinished.RemoveAll(this);
			Enemy->OnAttackAnimationFinished.AddUObject(this,&UBTTask_Attack::OnAttackFinishedCallback);

			Enemy->Attack();
			return EBTNodeResult::InProgress;
		}
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_Attack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyBase* Enemy = Cast<AEnemyBase>(OwnerComp.GetAIOwner()->GetPawn());
	if (Enemy)
	{
		Enemy->OnAttackAnimationFinished.RemoveAll(this);

		UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0.2f, Enemy->GetEnemyAttackMontage());
		}

		// [중요] 현재 상태가 Attack일 때만 Idle로 복구한다.
		// 피격/경직으로 인해 트리가 중단(Abort)된 경우, 이미 상태가 Hit/Stagger로
		// 바뀌어 있으므로 이를 Idle로 덮어쓰면 경직 애니메이션이 끊기는 버그가 발생한다.
		if (Enemy->GetCurrentState() == EEnemyState::Attack)
		{
			Enemy->SetState(EEnemyState::Idle);
		}
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_Attack::OnAttackFinishedCallback(EEnemyState NewState)
{
	if (CachedOwnerComp)
	{
		FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	}
}