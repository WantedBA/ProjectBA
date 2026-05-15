#include "Enemy/AI/BTDecorator_CheckHPThreshold.h"
#include "AIController.h"
#include "Enemy/EnemyBase.h"
#include "Component/StatComponent.h"

UBTDecorator_CheckHPThreshold::UBTDecorator_CheckHPThreshold()
{
	NodeName = TEXT("CheckHPThreshold");
}

bool UBTDecorator_CheckHPThreshold::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		return false;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(ControllingPawn);
	if (Enemy == nullptr)
	{
		return false;
	}

	UStatComponent* StatComp = Enemy->FindComponentByClass<UStatComponent>();
	if (StatComp == nullptr)
	{
		return false;
	}

	float HPRatio = StatComp->GetCurrentHP() / StatComp->GetMaxHP();

	if (bLessEqual)
	{
		return HPRatio <= HPThreshold;
	}
	else
	{
		return HPRatio > HPThreshold;
	}
}
