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
		if (DetectRange <= 0.0f)
		{
			DetectRange = 100000.0f; // 사실상 무한대
		}
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

	// [개선] 타겟 유실 및 복귀 로직
	// 공격 중(Attack), 피격(Hit/Stagger) 또는 경계(Alert) 중일 때는 타겟 유실 판정을 유보하여 상태 급변 방지
	bool bIsBusy = (Enemy->GetCurrentState() == EEnemyState::Attack || 
					Enemy->GetCurrentState() == EEnemyState::Hit || 
					Enemy->GetCurrentState() == EEnemyState::Stagger ||
					Enemy->GetCurrentState() == EEnemyState::Alert);

	if (NewTarget == nullptr && !bIsBusy)
	{
		// 이전에 타겟이 있었다가 사라진 경우 복귀 상태로 전환
		if (CurrentTarget != nullptr)
		{
			BBComp->SetValueAsBool(BBKey::IsReturning, true);
			Enemy->SetState(EEnemyState::Move);
			Enemy->ResetAttackCount(); // 복귀 시 공격 횟수 초기화
			UE_LOG(LogTemp, Log, TEXT("[%s] Target Actor Cleared -> Patrol/Return Mode"), *Enemy->GetName());
		}

		// 홈 위치 거리 체크: 이미 홈 근처라면 복귀 상태 해제
		FVector HomePos = BBComp->GetValueAsVector(BBKey::HomePos);
		float DistToHome = FVector::Dist(Center, HomePos);
		if (DistToHome < 150.0f) // 홈 반경 1.5m 이내면 도착으로 간주
		{
			BBComp->SetValueAsBool(BBKey::IsReturning, false);
		}

		BBComp->SetValueAsObject(BBKey::TargetActor, nullptr);
	}
	else if (NewTarget != nullptr)
	{
		BBComp->SetValueAsObject(BBKey::TargetActor, NewTarget);
		BBComp->SetValueAsBool(BBKey::IsReturning, false); // 타겟이 있으면 복귀 모드 해제
	}
	// bIsBusy인 경우 NewTarget이 null이라도 기존 TargetActor를 유지함 (상태 안정성)

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
	}

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
