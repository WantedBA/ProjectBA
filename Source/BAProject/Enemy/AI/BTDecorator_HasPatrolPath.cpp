#include "BTDecorator_HasPatrolPath.h"
#include "AIController.h"
#include "Components/SplineComponent.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
#include "Tables/PatrolPathRow.h"
#include "Enemy/EnemyBase.h"

UBTDecorator_HasPatrolPath::UBTDecorator_HasPatrolPath()
{
    NodeName = TEXT("HasPatrolPath");
}

bool UBTDecorator_HasPatrolPath::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

    APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (ControllingPawn == nullptr)
    {
        return false;
    }

    UBATableManager* TableManager = UBATableManager::Get(this);
    if (TableManager == nullptr)
    {
        return false;
    }

    AEnemyBase* Enemy = Cast<AEnemyBase>(ControllingPawn);
    if (Enemy)
    {
        uint32 MonstetTid = Enemy->GetMonsterTid();
        const FMonsterRows* MonsterRow = TableManager->FindMonster(MonstetTid);
        if (MonsterRow == nullptr)
            return false;

        uint32 PatrolPathTid = MonsterRow->PatrolPathTid;
        const FPatrolPathRow* PatrolPathRow = TableManager->FindPatrolPath(PatrolPathTid);
        if (PatrolPathRow == nullptr)
            return false;

        if (PatrolPathRow->Points.IsEmpty() == true)
            return false;

        return true;
    }

    return true;
}
