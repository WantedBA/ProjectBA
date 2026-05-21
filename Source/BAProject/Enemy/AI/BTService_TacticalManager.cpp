#include "Enemy/AI/BTService_TacticalManager.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"
#include "Enemy/EnemyBase.h"
#include "NavigationSystem.h"

UBTService_TacticalManager::UBTService_TacticalManager()
{
	NodeName = TEXT("TacticalManager");
	Interval = 0.3f;
	RandomDeviation = 0.1f;
}

void UBTService_TacticalManager::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return;
	}

	APawn* Pawn = AIController->GetPawn();
	UBlackboardComponent* BBComponent = OwnerComp.GetBlackboardComponent();
	if (Pawn == nullptr || BBComponent == nullptr)
	{
		return;
	}

	AActor* Target = Cast<AActor>(BBComponent->GetValueAsObject(BBKey::TargetActor));
	if (Target == nullptr)
	{
		return;
	}

	// 군무 방지: 개체별 Spacing 오프셋 최초 1회 생성 (Elden Ring 스타일의 거리감 분산)
	if (PersonalSpacingOffset == 0.0f)
	{
		PersonalSpacingOffset = FMath::FRandRange(-100.0f, 150.0f);
	}

	// 군무 방지: 방향 전환 타이밍 랜덤화 (칼군무 방지)
	float CurrentTime = Pawn->GetWorld()->GetTimeSeconds();
	if (CurrentTime >= NextDirectionChangeTime)
	{
		int32 NewDir = FMath::RandBool() ? 1 : -1;
		BBComponent->SetValueAsInt(BBKey::StrafeDir, NewDir);
		
		// 개체별로 1.5초 ~ 3.5초 사이에서 랜덤하게 방향 전환
		NextDirectionChangeTime = CurrentTime + FMath::FRandRange(1.5f, 3.5f);
	}

	// 전술 좌표 계산 (Strafing + Spacing)
	FVector OwnerLoc = Pawn->GetActorLocation();
	FVector TargetLoc = Target->GetActorLocation();
	FVector ToTarget = (TargetLoc - OwnerLoc).GetSafeNormal();
	FVector RightVector = FVector::CrossProduct(FVector::UpVector, ToTarget);

	float AttackRange = BBComponent->GetValueAsFloat(BBKey::AttackRange);
	int32 StrafeDir = BBComponent->GetValueAsInt(BBKey::StrafeDir);

	// 플레이어로부터 '공격 사거리 + 약간의 여유 + 개체별 오프셋' 만큼 떨어진 위치
	float TacticalDistance = AttackRange + 250.0f + PersonalSpacingOffset;
	FVector BaseTacticalPos = TargetLoc - (ToTarget * TacticalDistance);
	
	// 좌우로 서성이는 횡이동 좌표 추가
	FVector FinalTacticalPos = BaseTacticalPos + (RightVector * StrafeDir * 400.0f);

	// NavMesh 투영 (유효한 이동 경로 확보)
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld());
	if (NavSys)
	{
		FNavLocation ProjectedPos;
		if (NavSys->ProjectPointToNavigation(FinalTacticalPos, ProjectedPos, FVector(200.f, 200.f, 500.f)))
		{
			BBComponent->SetValueAsVector(BBKey::TacticalPos, ProjectedPos.Location);
		}
		else
		{
			// NavMesh 밖인 경우 즉시 방향 반전 시도 (벽에 막혔을 때)
			BBComponent->SetValueAsInt(BBKey::StrafeDir, StrafeDir * -1);
			NextDirectionChangeTime = CurrentTime + 1.0f; // 즉시 전환 방지 위해 약간의 유보
		}
	}
}
