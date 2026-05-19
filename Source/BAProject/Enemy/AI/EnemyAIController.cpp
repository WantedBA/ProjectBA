#include "Enemy/AI/EnemyAIController.h"
#include "Tables/BATableManager.h"
#include "Tables/MonsterRows.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Constants/BAProjectConstant.h"

AEnemyAIController::AEnemyAIController()
{
}

void AEnemyAIController::InitializeAI(int32 InTid, APawn* InPawn)
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
			UE_LOG(LogTemp, Log, TEXT("[AEnemyAIController] Blackboard initialized for Tid: %d"), InTid);
			if (Blackboard)
			{
				Blackboard->SetValueAsInt(TEXT("MonsterTid"), InTid);
				
				APawn* ControlledPawn = InPawn ? InPawn : (APawn*)GetPawn();
				if (ControlledPawn)
				{
					Blackboard->SetValueAsVector(BBKey::HomePos, ControlledPawn->GetActorLocation());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[AEnemyAIController] Pawn is null in InitializeAI (InPawn and GetPawn() are both null)"));
				}

				Blackboard->SetValueAsFloat(BBKey::DetectRange, static_cast<float>(MonsterRow->DetectRange));
				Blackboard->SetValueAsFloat(BBKey::AttackRange, static_cast<float>(MonsterRow->AttackRange));
				Blackboard->SetValueAsFloat(BBKey::ReturnRange, static_cast<float>(MonsterRow->DetectRange) * 2.5f);
				Blackboard->SetValueAsBool(BBKey::IsReturning, false);
				Blackboard->SetValueAsBool(BBKey::IsActionLocked, false);
			}

			bool bStarted = RunBehaviorTree(BTAsset);
			UE_LOG(LogTemp, Log, TEXT("[AEnemyAIController] RunBehaviorTree result: %s"), bStarted ? TEXT("True") : TEXT("False"));

			if (bStarted && Blackboard)
			{
				int32 CheckTid = Blackboard->GetValueAsInt(TEXT("MonsterTid"));
				UE_LOG(LogTemp, Log, TEXT("[AEnemyAIController] Blackboard Verified MonsterTid: %d"), CheckTid);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AEnemyAIController] Failed to load assets. BBAsset: %s, BTAsset: %s"), 
			BBAsset ? *BBAsset->GetName() : TEXT("Null"), 
			BTAsset ? *BTAsset->GetName() : TEXT("Null"));
	}
}
