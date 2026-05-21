#include "Enemy/AI/BTTask_PlayMontageWithState.h"
#include "AIController.h"
#include "Enemy/EnemyBase.h"
#include "Animation/AnimInstance.h"

UBTTask_PlayMontageWithState::UBTTask_PlayMontageWithState()
{
	NodeName = TEXT("PlayMontageWithState");
	TargetState = EEnemyState::Hit;
}

EBTNodeResult::Type UBTTask_PlayMontageWithState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(AIController->GetPawn());
	if(Enemy == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// 현재 상태가 TargetState인지 확인 (EnemyBase::SetState에서 이미 몽타주 재생을 시작했을 수 있음)
	// 하지만 Task 차원에서 동기화를 위해 여기서 다시 한번 체크 및 재생 보장
	UAnimMontage* MontageToPlay = nullptr;
	if (TargetState == EEnemyState::Hit)
	{
		MontageToPlay = Enemy->GetEnemyHitMontage();
	}
	else if (TargetState == EEnemyState::Stagger)
	{
		MontageToPlay = Enemy->GetEnemyStaggerMontage();
	}

	if (!MontageToPlay)
	{
		Enemy->SetState(EEnemyState::Idle);
		return EBTNodeResult::Succeeded;
	}

	UAnimInstance* AnimInstance = Enemy->GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		// 델리게이트 바인딩
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UBTTask_PlayMontageWithState::OnMontageFinished, TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp));
		AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

		// 이미 재생 중이 아니라면 재생
		if (AnimInstance->Montage_IsPlaying(MontageToPlay) == false)
		{
			Enemy->PlayAnimMontage(MontageToPlay);
		}
		
		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_PlayMontageWithState::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		AEnemyBase* Enemy = Cast<AEnemyBase>(AIController->GetPawn());
		if (Enemy)
		{
			Enemy->GetMesh()->GetAnimInstance()->Montage_Stop(0.2f);
			if (Enemy->GetCurrentState() != EEnemyState::Dead)
			{
				Enemy->SetState(EEnemyState::Idle);
			}
		}
	}
	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_PlayMontageWithState::OnMontageFinished(UAnimMontage* Montage, bool bInterrupted, TWeakObjectPtr<UBehaviorTreeComponent> OwnerCompPtr)
{
	if (OwnerCompPtr.IsValid())
	{
		AAIController* AIController = OwnerCompPtr->GetAIOwner();
		if (AIController)
		{
			AEnemyBase* Enemy = Cast<AEnemyBase>(AIController->GetPawn());
			if (Enemy)
			{
				if (Enemy->GetCurrentState() != EEnemyState::Dead)
				{
					Enemy->SetState(EEnemyState::Idle);
				}
			}
		}
		FinishLatentTask(*OwnerCompPtr, EBTNodeResult::Succeeded);
	}
}
