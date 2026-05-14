#include "Enemy/AI/BTService_DetectTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Constants/BAProjectConstant.h"

UBTService_DetectTarget::UBTService_DetectTarget()
{
    NodeName = TEXT("DetectTarget");
    Interval = 0.5f; // 기본 체크 주기 설정
}

void UBTService_DetectTarget::TickNode(UBehaviorTreeComponent & OwnerComp, uint8 * NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (ControllingPawn == nullptr)
    {
        return;
    }

    UWorld* World = ControllingPawn->GetWorld();
    if (World == nullptr)
    {
        return;
    }

    UBlackboardComponent* BBComponent = OwnerComp.GetBlackboardComponent();
    if (BBComponent == nullptr)
    {
        return;
    }

    bool bIsReturning = BBComponent->GetValueAsBool(BBKey::IsReturning);
    FVector HomePos = BBComponent->GetValueAsVector(BBKey::HomePos);
    float DistanceFromHome = FVector::Dist(ControllingPawn->GetActorLocation(), HomePos);

    if (bIsReturning)
    {
        // 복귀 중O, 거리 체크
        if (DistanceFromHome <= 100.f)
        {
            BBComponent->SetValueAsBool(BBKey::IsReturning, false);
            bIsReturning = false;
        }
        // 복귀 중X, 타겟 초기화
        else
        {
            BBComponent->SetValueAsObject(BBKey::TargetActor, nullptr);
            return;
        }
    }

    // 타겟 있을때만 ReturnRange를 체크 절대 복귀 모드 전환 여부 결정
    AActor* CurrentTarget = Cast<AActor>(BBComponent->GetValueAsObject(BBKey::TargetActor));
    if (CurrentTarget != nullptr)
    {
        float ReturnRange = BBComponent->GetValueAsFloat(BBKey::ReturnRange);
        if (ReturnRange > 0.f && DistanceFromHome > ReturnRange)
        {
            BBComponent->SetValueAsBool(BBKey::IsReturning, true);
            BBComponent->SetValueAsObject(BBKey::TargetActor, nullptr);
            return;
        }
    }

    // 테이블에서 로드된 감지 범위를 Blackboard에서 가져옴
    float Range = BBComponent->GetValueAsFloat(BBKey::DetectRange);
    if (Range <= 0.f)
    {
        Range = 500.f; // 기본값
    }

    // 0번 플레이어(싱글 플레이어 기준) 타겟팅
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(World, 0);
    if (PlayerCharacter == nullptr)
    {
        BBComponent->SetValueAsObject(BBKey::TargetActor, nullptr);
        return;
    }

    float Distance = FVector::Dist(PlayerCharacter->GetActorLocation(), ControllingPawn->GetActorLocation());

    // 상수 키 값을 사용하여 타겟 설정
    if (Distance <= Range)
    {
        BBComponent->SetValueAsObject(BBKey::TargetActor, PlayerCharacter);
    }
    else
    {
        BBComponent->SetValueAsObject(BBKey::TargetActor, nullptr);
    }
}
