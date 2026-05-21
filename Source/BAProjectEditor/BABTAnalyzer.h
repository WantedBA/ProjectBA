// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BAAIAnalyzerData.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTNode.h"
#include "BABTAnalyzer.generated.h"

UCLASS()
class UBABTAnalyzer : public UObject
{
	GENERATED_BODY()

public:
	UBABTAnalyzer();

	bool AnalyzeBehaviorTree(UBehaviorTree* InTree, FBAAIAnalyzerTreeData& OutData);

private:
	FString ProcessNode(UBTNode* InNode, FBAAIAnalyzerTreeData& OutData);
	EBAAIAnalyzerNodeType GetNodeType(UBTNode* InNode);
};
