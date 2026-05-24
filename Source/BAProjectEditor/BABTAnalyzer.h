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
	FString ProcessNode(UBTNode* InNode, FBAAIAnalyzerTreeData& OutData, const FString& ParentId, int32 Depth, int32 ChildIndex);
	EBAAIAnalyzerNodeType GetNodeType(UBTNode* InNode);
	EBAAICompositeLogic GetCompositeLogic(UBTNode* InNode);
	FString ExtractSemanticIntent(UBTNode* InNode);
	FString ExtractBlackboardAccessType(const FString& PropertyName, UBTNode* Node);
};
