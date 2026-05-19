#include "Enemy/AI/BTService_UpdateTargetContext.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Enemy/EnemyBase.h"

UBTService_UpdateTargetContext::UBTService_UpdateTargetContext()
{
	NodeName = TEXT("UpdateTargetContext");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTService_UpdateTargetContext::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		return;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(ControllingPawn);
	if (Enemy == nullptr)
	{
		return;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return;
	}

	float DetectRange = BBComp->GetValueAsFloat(BBKey::DetectRange);
	if (Enemy->IsPersistentAggro())
	{
		if (DetectRange <= 0.0f)
			DetectRange = UE_BIG_NUMBER;
	}
	
	FVector Center = ControllingPawn->GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params(NAME_None, false, ControllingPawn);

	bool bResult = ControllingPawn->GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		Center,
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn,
		FCollisionShape::MakeSphere(DetectRange),
		Params
	);

	AActor* CurrentTarget = Cast<AActor>(BBComp->GetValueAsObject(BBKey::TargetActor));
	AActor* NewTarget = nullptr;

	if (bResult)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			APawn* Pawn = Cast<APawn>(Result.GetActor());
			if (Pawn == nullptr)
			{
				continue;
			}

			if (Pawn == ControllingPawn)
			{
				continue;
			}

			if (Pawn->IsPlayerControlled())
			{
				NewTarget = Pawn;
				break;
			}
		}
	}

	// 타겟 유지 정책: 끈질긴 어그로는 살아있는 한 무조건 유지, 그 외는 거리 fallback
	if (NewTarget == nullptr && CurrentTarget != nullptr)
	{
		bool bShouldKeep = false;

		if (Enemy->IsPersistentAggro())
		{
			// 끈질긴 어그로: 타겟이 살아있기만 하면 영구 유지 (시야/거리 무관)
			AEnemyBase* TargetEnemy = Cast<AEnemyBase>(CurrentTarget);
			bool bTargetDead = TargetEnemy ? TargetEnemy->IsDead() : false;
			bShouldKeep = !bTargetDead && IsValid(CurrentTarget);
		}
		else
		{
			// 일반 몬스터: 감지 범위의 1.5배 안에 있으면 유지
			float DistToOldTarget = FVector::Dist(Center, CurrentTarget->GetActorLocation());
			bShouldKeep = (DistToOldTarget < DetectRange * 1.5f);
		}

		if (bShouldKeep)
		{
			NewTarget = CurrentTarget;
		}
	}

	BBComp->SetValueAsObject(BBKey::TargetActor, NewTarget);

	// 거리, 각도 체크
	if (NewTarget)
	{
		FVector TargetLoc = NewTarget->GetActorLocation();
		float Distance = FVector::Dist(Center, TargetLoc);
		BBComp->SetValueAsFloat(BBKey::TargetDistance, Distance);

		// 각도 계산
		FVector Forward = ControllingPawn->GetActorForwardVector();
		FVector ToTarget = (TargetLoc - Center).GetSafeNormal();
		float Dot = FVector::DotProduct(Forward, ToTarget); // 정면 1, 뒤 -1, 좌우 0
		float Det = Forward.X * ToTarget.Y - Forward.Y * ToTarget.X; // 좌, 우 판별
		float AngleToTarget = FMath::Atan2(Det, Dot) * (180.0f / PI); // 0 정면, 90 왼쪽, -90 오른쪽, 180/-180 뒤

		BBComp->SetValueAsFloat(BBKey::TargetAngle, AngleToTarget);
#if WITH_EDITOR

		// 방향 Debug
		//DrawDebugLine(
		//	ControllingPawn->GetWorld(),
		//	Center,
		//	Center + Forward * 200.f,
		//	FColor::magenta,
		//	false,
		//	0.5f,
		//	0,
		//	3.f
		//);

		// 위치
		//DrawDebugLine(
		//	ControllingPawn->GetWorld(),
		//	Center,
		//	TargetLoc,
		//	FColor::Yellow,
		//	false,
		//	0.5f,
		//	0,
		//	2.f
		//);

#endif
	}
}
