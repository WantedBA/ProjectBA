#include "Enemy/AI/BTTask_SelectPattern_Utility.h"
#include "AIController.h"
#include "Enemy/Boss.h"
#include "Component/StatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"

UBTTask_SelectPattern_Utility::UBTTask_SelectPattern_Utility()
{
	NodeName = TEXT("SelectPattern_Utility");
}

EBTNodeResult::Type UBTTask_SelectPattern_Utility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	UStatComponent* StatComp = Boss->FindComponentByClass<UStatComponent>();
	if (StatComp == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(BBKey::TargetActor));
	if (Target == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	float HPRatio = StatComp->GetCurrentHP() / StatComp->GetMaxHP();
	float DistanceToTarget = FVector::Dist(Boss->GetActorLocation(), Target->GetActorLocation());
	float AngleToTarget = BBComp->GetValueAsFloat(BBKey::TargetAngle);

	struct FPatternCandidate
	{
		int32 Index;
		float FinalScore;
		int32 IdealRange;
	};

	TArray<FPatternCandidate> Candidates;
	float TotalScore = 0.0f;

	const TArray<FBossAttackData>& Patterns = Boss->GetBossPatterns();

	for (int32 i = 0; i < Patterns.Num(); ++i)
	{
		const auto& Row = Patterns[i];

		if (Boss->IsPatternAvailable(Row.Tid) == false)
		{
			continue;
		}

		// HP조건, % 이하일 때 발동
		if (Row.ConditionType == 1 && (HPRatio * 100.0f) > Row.Var1)
		{
			continue;
		}

		float Score = Row.Weight * Row.ScoreMultiplier;

		// 거리 점수
		float RangeDiff = FMath::Abs(DistanceToTarget - Row.IdealRange);
		float RangeFactor = FMath::Clamp(1.0f - (RangeDiff / 1500.0f), 0.1f, 1.2f);
		Score *= RangeFactor;

		// 각도/대각선 점수
		float AngleDiff = FMath::Abs(FMath::Abs(AngleToTarget) - Row.AttackAngle);
		if (AngleDiff < 20.0f) 
		{
			Score *= 1.5f; 
		}

		if (Score > 0)
		{
			// 너무 먼 거리의 패턴은 후보에서 제외
			if (DistanceToTarget > Row.IdealRange * 2.0f)
			{
				continue;
			}

			Candidates.Add({ i, Score, Row.IdealRange });
			TotalScore += Score;
		}
	}

	if (Candidates.Num() == 0 || TotalScore <= 0.0f)
	{
		// 가능한 패턴이 없을 때 처리 (예: 기본 공격 인덱스 강제 지정 등)
		UE_LOG(LogTemp, Warning, TEXT("No available patterns for Boss!"));
		return EBTNodeResult::Failed;
	}

	float RandomValue = FMath::FRandRange(0.0f, TotalScore);
	float AccScore = 0.0f;

	for (const auto& Candidate : Candidates)
	{
		AccScore += Candidate.FinalScore;
		if (RandomValue <= AccScore)
		{
			BBComp->SetValueAsInt(BBKey::SelectedPatternIndex, Candidate.Index);
			BBComp->SetValueAsFloat(BBKey::TargetDistance, static_cast<float>(Candidate.IdealRange));
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
