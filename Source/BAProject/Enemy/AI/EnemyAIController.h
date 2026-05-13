#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

UCLASS()
class BAPROJECT_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	void InitializeAI(int32 InTid, APawn* InPawn = nullptr);
	void StartAI();
	void StopAI();

private:
	UPROPERTY()
	TObjectPtr<UBlackboardData> BBAsset;

	UPROPERTY()
	TObjectPtr<UBehaviorTree> BTAsset;

	UPROPERTY()
	int32 Tid;
};
