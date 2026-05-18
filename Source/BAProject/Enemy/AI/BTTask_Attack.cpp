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

		if (Enemy->GetCurrentState() != EEnemyState::Dead)
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