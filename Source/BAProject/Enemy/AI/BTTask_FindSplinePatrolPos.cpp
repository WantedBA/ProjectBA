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
		UE_LOG(LogTemp, Error, TEXT("ControllingPawn is Null"));
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
		UE_LOG(LogTemp, Warning, TEXT("[%s] MonsterRow or PatrolPathTid is invalid. Tid: %d, PathTid: %d"), 
			*ControllingPawn->GetName(), MonsterTid, MonsterRow ? MonsterRow->PatrolPathTid : -1);
		return EBTNodeResult::Failed;
	}

	const FPatrolPathRow* PathRow = TableManager->FindPatrolPath(MonsterRow->PatrolPathTid);
	if (PathRow == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] PatrolPathRow not found for Tid: %d"), *ControllingPawn->GetName(), MonsterRow->PatrolPathTid);
		return EBTNodeResult::Failed;
	}

	if (PathRow->Points.Num() > 0)
	{
		int32 CurrentIndex = BBComponent->GetValueAsInt(BBKey::SplineIndex);
		int32 NextIndex = (CurrentIndex + 1) % PathRow->Points.Num();

		FVector NextPos = PathRow->Points[NextIndex];
		BBComponent->SetValueAsVector(BBKey::PatrolPos, NextPos);
		BBComponent->SetValueAsInt(BBKey::SplineIndex, NextIndex);

		return EBTNodeResult::Succeeded;
	}

	if (PathRow->PathTag.IsNone() == false)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), PathRow->PathTag, FoundActors);
		
		if (FoundActors.Num() > 0)
		{
			APatrolPath* PathActor = Cast<APatrolPath>(FoundActors[0]);
			if (PathActor)
			{
				USplineComponent* Spline = PathActor->GetSplineComponent();

				// 1. 현재 스플라인 상의 진행 거리 가져오기, 처음 시작하거나 경로 이탈 후 복귀 시에는 -1.0f 등의 기본값임을 가정
				float CurrentDistance = BBComponent->GetValueAsFloat(BBKey::SplineDistance);

				// 2. 복귀 로직: 만약 경로를 새로 시작하거나 멀리 떨어져 있다면 가장 가까운 거리 찾기
				// (여기서는 단순화를 위해 처음 0인 상태를 복귀 혹은 시작점으로 간주하거나 거리 체크 로직 추가 가능)
				if (CurrentDistance <= 0.0f)
				{
					float ClosestInputKey = Spline->FindInputKeyClosestToWorldLocation(ControllingPawn->GetActorLocation());
					CurrentDistance = Spline->GetDistanceAlongSplineAtSplineInputKey(ClosestInputKey);
				}

				// 3. 샘플링: 현재 위치에서 일정 거리(예: 50cm) 앞의 좌표를 목표로 설정
				// 이 간격이 좁을수록 곡선에 더 밀착하지만, MoveTo가 너무 빈번해질 수 있음
				float TargetDistance = CurrentDistance + 50.0f;

				// 4. 루프 처리
				float TotalLength = Spline->GetSplineLength();
				if (TargetDistance >= TotalLength)
				{
					TargetDistance = FMath::Fmod(TargetDistance, TotalLength);
				}

				// 5. 좌표 추출 및 블랙보드 갱신
				FVector NextPos = Spline->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);

				BBComponent->SetValueAsVector(BBKey::PatrolPos, NextPos);
				BBComponent->SetValueAsFloat(BBKey::SplineDistance, TargetDistance);

				return EBTNodeResult::Succeeded;
			}
		}
	}

	return EBTNodeResult::Failed;
}
