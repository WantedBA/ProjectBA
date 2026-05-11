#include "Enemy/AI/EnemyAIController.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"

AEnemyAIController::AEnemyAIController()
{
}

void AEnemyAIController::InitializeAI(int32 InTid)
{
	Tid = InTid;

	UBATableManager* TableManager = UBATableManager::Get(this);
	if (TableManager == nullptr)
	{
		return;
	}

	const FMonsterRows* MonsterRow = TableManager->FindMonster(InTid);
	if (MonsterRow == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AEnemyAIController] Monster data not found for Tid: %d"), InTid);
		return;
	}

	if (!MonsterRow->BBPath.IsEmpty())
	{
		BBAsset = Cast<UBlackboardData>(StaticLoadObject(UBlackboardData::StaticClass(), nullptr, *MonsterRow->BBPath));
	}

	if (!MonsterRow->BTPath.IsEmpty())
	{
		BTAsset = Cast<UBehaviorTree>(StaticLoadObject(UBehaviorTree::StaticClass(), nullptr, *MonsterRow->BTPath));
	}

	if (BBAsset && BTAsset)
	{
		UBlackboardComponent* BBComp = Blackboard;
		if (UseBlackboard(BBAsset, BBComp))
		{
			RunBehaviorTree(BTAsset);
		}
	}
}

void AEnemyAIController::StartAI()
{
}

void AEnemyAIController::StopAI()
{
}
