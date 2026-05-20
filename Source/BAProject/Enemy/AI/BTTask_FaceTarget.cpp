#include "Enemy/AI/BTTask_FaceTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"
#include "Enemy/Boss.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

UBTTask_FaceTarget::UBTTask_FaceTarget()
{
	NodeName = TEXT("Face Target");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_FaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (AIController == nullptr || BB == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	ABoss* Boss = Cast<ABoss>(AIController->GetPawn());
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(BBKey::TargetActor));
	if (Boss == nullptr || Target == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UAnimMontage* TurnMontage = Boss->PlayTurnToTarget(Target);
	if (TurnMontage == nullptr)
	{
		// 회전 불필요(이미 정면) — Failed 반환해 Selector가 다음 분기(Idle 등)로 진행하게 함
		return EBTNodeResult::Failed;
	}

	// 회전 몽타주 재생 중 — 종료까지 대기 (TickTask 폴링)
	return EBTNodeResult::InProgress;
}

void UBTTask_FaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ACharacter* Boss = AIController ? Cast<ACharacter>(AIController->GetPawn()) : nullptr;
	if (Boss == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	USkeletalMeshComponent* MeshComp = Boss->GetMesh();
	UAnimInstance* AnimInst = MeshComp ? MeshComp->GetAnimInstance() : nullptr;

	// 회전 몽타주가 끝났으면(또는 AnimInstance 없으면) 완료
	if (AnimInst == nullptr || !AnimInst->IsAnyMontagePlaying())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
