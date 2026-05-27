#include "Enemy/AI/BTTask_SelectPattern_Utility.h"
#include "AIController.h"
#include "Enemy/Boss.h"
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

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (BBComp == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	int32 SelectedPatternTid = Boss->ChooseBestPattern();
	UE_LOG(LogTemp, Warning, TEXT("[SelectPattern] ChooseBestPattern returned Tid=%d"), SelectedPatternTid);

	if (SelectedPatternTid != 0)
	{
		BBComp->SetValueAsInt(BBKey::SelectedPatternTid, SelectedPatternTid);

		// 선택된 패턴의 IdealRange도 BB에 박아서 MoveToRange/CheckPatternRange가 사용
		const float PatternIdealRange = Boss->GetPatternIdealRange(SelectedPatternTid);
		BBComp->SetValueAsFloat(BBKey::SelectedPatternIdealRange, PatternIdealRange);

		// 직후 다시 읽어서 BB에 정상적으로 박혔는지 검증
		const int32 BBVerifyTid = BBComp->GetValueAsInt(BBKey::SelectedPatternTid);
		UE_LOG(LogTemp, Warning, TEXT("  → SetValueAsInt done, BB readback Tid=%d, Range=%f"), BBVerifyTid, PatternIdealRange);

		return EBTNodeResult::Succeeded;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SelectPattern] FAILED - Tid=0 from ChooseBestPattern"));
	return EBTNodeResult::Failed;
}
