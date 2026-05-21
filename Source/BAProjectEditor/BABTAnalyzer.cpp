// Copyright TeamBA. All Rights Reserved.

#include "BABTAnalyzer.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BlackboardData.h"

UBABTAnalyzer::UBABTAnalyzer()
{}

bool UBABTAnalyzer::AnalyzeBehaviorTree(UBehaviorTree* InTree, FBAAIAnalyzerTreeData& OutData)
{
	if (!InTree || !InTree->RootNode)
	{
		return false;
	}

	OutData.TreeName = InTree->GetName();
	if (InTree->BlackboardAsset)
	{
		OutData.BlackboardName = InTree->BlackboardAsset->GetName();

		// Extract Blackboard Keys
		for (int32 i = 0; i < InTree->BlackboardAsset->GetNumKeys(); ++i)
		{
			const FBlackboardEntry* Entry = InTree->BlackboardAsset->GetKey(i);
			if (Entry)
			{
				FBAAIAnalyzerBlackboardKeyData KeyData;
				KeyData.KeyName = Entry->EntryName.ToString();
				if (Entry->KeyType)
				{
					KeyData.KeyType = Entry->KeyType->GetClass()->GetName();
					// Remove 'BlackboardKeyType_' prefix if present for cleaner output
					KeyData.KeyType.RemoveFromStart(TEXT("BlackboardKeyType_"));
				}
				OutData.BlackboardKeys.Add(KeyData);
			}
		}
	}

	OutData.RootNodeId = ProcessNode(InTree->RootNode, OutData);

	return true;
}

FString UBABTAnalyzer::ProcessNode(
	UBTNode* InNode,
	FBAAIAnalyzerTreeData& OutData)
{
	if (!InNode)
	{
		return FString();
	}

	// Stable ID
	const FString NodeId = FString::Printf(
		TEXT("%s_%d"),
		*InNode->GetNodeName(),
		InNode->GetExecutionIndex());

	// Prevent recursive duplication
	if (OutData.Nodes.Contains(NodeId))
	{
		return NodeId;
	}

	FBAAIAnalyzerNodeData NodeData;

	NodeData.NodeId = NodeId;
	NodeData.NodeName = InNode->GetNodeName();
	NodeData.NodeType = GetNodeType(InNode);

	// Composite processing
	if (UBTCompositeNode* CompositeNode =
		Cast<UBTCompositeNode>(InNode))
	{
		for (int32 ChildIndex = 0;
			ChildIndex < CompositeNode->Children.Num();
			++ChildIndex)
		{
			FBTCompositeChild& Child =
				CompositeNode->Children[ChildIndex];

			// Child node
			if (UBTNode* ChildNode =
				CompositeNode->GetChildNode(ChildIndex))
			{
				NodeData.ChildrenIds.Add(
					ProcessNode(ChildNode, OutData));
			}

			// Decorators attached to child
			for (UBTDecorator* Decorator :
				Child.Decorators)
			{
				if (Decorator)
				{
					NodeData.AttachedDecoratorIds.Add(
						ProcessNode(Decorator, OutData));
				}
			}
		}

		// Services attached to composite
		for (UBTService* Service :
			CompositeNode->Services)
		{
			if (Service)
			{
				NodeData.AttachedServiceIds.Add(
					ProcessNode(Service, OutData));
			}
		}
	}

	OutData.Nodes.Add(NodeId, NodeData);

	return NodeId;
}

EBAAIAnalyzerNodeType UBABTAnalyzer::GetNodeType(
	UBTNode* InNode)
{
	if (!InNode)
	{
		return EBAAIAnalyzerNodeType::Unknown;
	}

	if (Cast<UBTCompositeNode>(InNode))
	{
		return EBAAIAnalyzerNodeType::Composite;
	}

	if (Cast<UBTTaskNode>(InNode))
	{
		return EBAAIAnalyzerNodeType::Task;
	}

	if (Cast<UBTDecorator>(InNode))
	{
		return EBAAIAnalyzerNodeType::Decorator;
	}

	if (Cast<UBTService>(InNode))
	{
		return EBAAIAnalyzerNodeType::Service;
	}

	return EBAAIAnalyzerNodeType::Unknown;
}