#include "Enemy/Animation/ANS_RotateTracking.h"
#include "Enemy/EnemyBase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"
#include "Kismet/KismetMathLibrary.h"

void UANS_RotateTracking::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
	if (Enemy == nullptr)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(Enemy->GetController());
	if (AIController == nullptr)
	{
		return;
	}

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (Blackboard == nullptr)
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BBKey::TargetActor));
	if (TargetActor == nullptr)
	{
		return;
	}

	FVector LookDir = TargetActor->GetActorLocation() - Enemy->GetActorLocation();
	LookDir.Z = 0.0f;

	if (LookDir.IsNearlyZero())
	{
		return;
	}

	FRotator TargetRotation = LookDir.Rotation();
	FRotator CurrentRotation = Enemy->GetActorRotation();

	FRotator NewRotation = UKismetMathLibrary::RInterpTo(CurrentRotation, TargetRotation, FrameDeltaTime, RotationSpeed);
	Enemy->SetActorRotation(NewRotation);
}

void UANS_RotateTracking::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner());
	if (Enemy)
	{
		AAIController* AIController = Enemy->GetController<AAIController>();
		if (AIController && AIController->GetBlackboardComponent())
		{
			// 공격 추적이 끝났으므로 락을 해제하여 BT가 다시 판단하게 함
			AIController->GetBlackboardComponent()->SetValueAsBool(BBKey::IsActionLocked, false);
		}
	}
}
