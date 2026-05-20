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
	if (SelectedPatternTid != 0)
	{
		BBComp->SetValueAsInt(BBKey::SelectedPatternTid, SelectedPatternTid);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
