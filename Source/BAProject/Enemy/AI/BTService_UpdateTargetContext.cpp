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
	if (Enemy->GetEnemyGrade() == EEnemyGrade::Boss)
	{
		DetectRange = 1000.0f;
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

	DrawDebugSphere(
		ControllingPawn->GetWorld(),
		Center,
		DetectRange,
		16,
		bResult ? FColor::Green : FColor::Red,
		false,
		0.5f
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

	if (NewTarget == nullptr && CurrentTarget != nullptr)
	{
		float DistToOldTarget = FVector::Dist(Center, CurrentTarget->GetActorLocation());
		if (DistToOldTarget < DetectRange * 1.5f) // 감지 범위보다 조금 더 여유를 둠
		{
			NewTarget = CurrentTarget;
		}
	}

	BBComp->SetValueAsObject(BBKey::TargetActor,NewTarget);

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
		DrawDebugLine(
			ControllingPawn->GetWorld(),
			Center,
			Center + Forward * 200.f,
			FColor::Blue,
			false,
			0.5f,
			0,
			3.f
		);

		DrawDebugLine(
			ControllingPawn->GetWorld(),
			Center,
			TargetLoc,
			FColor::Yellow,
			false,
			0.5f,
			0,
			2.f
		);

#endif
	}
}
