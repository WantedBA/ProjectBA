#include "Enemy/AI/BTTask_FindSplinePatrolPos.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/AI/PatrolPath.h"
#include "Constants/BAProjectConstant.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
#include "Kismet/GameplayStatics.h"

UBTTask_FindSplinePatrolPos::UBTTask_FindSplinePatrolPos()
{
	NodeName = TEXT("FindSplinePatrolPos");
}

EBTNodeResult::Type UBTTask_FindSplinePatrolPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BBComponent = OwnerComp.GetBlackboardComponent();
	if (BBComponent == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UBATableManager* TableManager = UBATableManager::Get(ControllingPawn);
	if (TableManager == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	int32 MonsterTid = BBComponent->GetValueAsInt(TEXT("MonsterTid"));
	const FMonsterRows* MonsterRow = TableManager->FindMonster(MonsterTid);
	if (MonsterRow == nullptr || MonsterRow->PatrolPathTid == 0)
	{
		return EBTNodeResult::Failed;
	}

	const FPatrolPathRow* PathRow = TableManager->FindPatrolPath(MonsterRow->PatrolPathTid);
	if (PathRow == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTTask_FindSplinePatrolPos] PatrolPath data not found for Tid: %d"), MonsterRow->PatrolPathTid);
		return EBTNodeResult::Failed;
	}

	if (PathRow->Points.Num() > 0)
	{
		int32 CurrentIndex = BBComponent->GetValueAsInt(BBKey::SplineIndex);
		int32 NextIndex = (CurrentIndex + 1) % PathRow->Points.Num();

		BBComponent->SetValueAsVector(BBKey::PatrolPos, PathRow->Points[NextIndex]);
		BBComponent->SetValueAsInt(BBKey::SplineIndex, NextIndex);
		return EBTNodeResult::Succeeded;
	}

	if (!PathRow->PathTag.IsNone())
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), PathRow->PathTag, FoundActors);
		
		if (FoundActors.Num() > 0)
		{
			APatrolPath* PathActor = Cast<APatrolPath>(FoundActors[0]);
			if (PathActor)
			{
				USplineComponent* Spline = PathActor->GetSplineComponent();
				int32 MaxPoints = Spline->GetNumberOfSplinePoints();

				int32 CurrentIndex = BBComponent->GetValueAsInt(BBKey::SplineIndex);
				int32 NextIndex = (CurrentIndex + 1) % MaxPoints;

				FVector NextPos = Spline->GetLocationAtSplinePoint(NextIndex, ESplineCoordinateSpace::World);

				BBComponent->SetValueAsVector(BBKey::PatrolPos, NextPos);
				BBComponent->SetValueAsInt(BBKey::SplineIndex, NextIndex);
				return EBTNodeResult::Succeeded;
			}
		}
	}

	return EBTNodeResult::Failed;
}
