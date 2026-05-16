#include "Enemy/AI/BTDecorator_CheckPatternRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Constants/BAProjectConstant.h"

UBTDecorator_CheckPatternRange::UBTDecorator_CheckPatternRange()
{
	NodeName = TEXT("CheckPatternRange");
}

bool UBTDecorator_CheckPatternRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return false;
	}

	APawn* Boss = AIController->GetPawn();
	if (Boss == nullptr)
	{
		return false;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return false;
	}

	AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(BBKey::TargetActor));
	if (Target == nullptr)
	{
		return false;
	}

	float TargetDist = BBComp->GetValueAsFloat(BBKey::TargetDistance);
	float Distance = FVector::Dist(Boss->GetActorLocation(), Target->GetActorLocation());

	// [수정] 현재 거리가 목표 사거리보다 '멀 때' 이 시퀀스(이동)를 실행함
	return Distance > (TargetDist + AcceptanceRadius);
}
