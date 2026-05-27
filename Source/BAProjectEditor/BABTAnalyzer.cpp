// Copyright TeamBA. All Rights Reserved.

#include "BABTAnalyzer.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTreeTypes.h"

UBABTAnalyzer::UBABTAnalyzer()
{
}

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

	OutData.RootNodeId = ProcessNode(InTree->RootNode,OutData,TEXT(""),0,0);

	return true;
}

FString UBABTAnalyzer::ProcessNode(UBTNode* InNode, FBAAIAnalyzerTreeData& OutData, const FString& ParentId, int32 Depth, int32 ChildIndex)
{
	if (InNode == nullptr)
	{
		return FString();
	}

	// Stable ID
	const FString NodeId = FString::Printf(TEXT("%s_%s_%d"), *InNode->GetClass()->GetName(), *InNode->GetNodeName(), InNode->GetExecutionIndex());

	// Prevent recursive duplication
	if (OutData.Nodes.Contains(NodeId))
	{
		return NodeId;
	}

	FBAAIAnalyzerNodeData NodeData;

	NodeData.NodeId = NodeId;
	NodeData.ParentNodeId = ParentId;
	NodeData.Depth = Depth;
	NodeData.ChildIndex = ChildIndex;

	NodeData.NodeName = InNode->GetNodeName();
	NodeData.NodeType = GetNodeType(InNode);

	NodeData.CompositeLogic = GetCompositeLogic(InNode);

	// 노드의 고유 실행 인덱스 및 상세 설명
	NodeData.CustomProperties.Add(TEXT("ExecutionIndex"), FString::FromInt(InNode->GetExecutionIndex()));
	NodeData.CustomProperties.Add(TEXT("StaticDescription"), InNode->GetStaticDescription());
	NodeData.CustomProperties.Add(TEXT("SemanticIntent"), ExtractSemanticIntent(InNode));
	if (const FBoolProperty* CreateInstanceProp =FindFProperty<FBoolProperty>(InNode->GetClass(),TEXT("bCreateNodeInstance")))
	{
		const bool bCreateInstance =CreateInstanceProp->GetPropertyValue_InContainer(InNode);
		NodeData.CustomProperties.Add(TEXT("bCreateNodeInstance"),bCreateInstance ? TEXT("True") : TEXT("False"));
	}
	NodeData.CustomProperties.Add(TEXT("ClassName"),InNode->GetClass()->GetName());

	// 리플렉션을 통해 노드가 참조하는 Blackboard Key 구조체 자동 추출
	for (TFieldIterator<FProperty> PropIt(InNode->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (Property == nullptr)
		{
			continue;
		}
		// FBlackboardKeySelector 타입을 사용하는 프로퍼티 검색
		FStructProperty* StructProp = CastField<FStructProperty>(Property);
		if (StructProp == nullptr)
		{
			continue;
		}

		if (StructProp->Struct->GetFName() == FName(TEXT("BlackboardKeySelector")))
		{
			FBlackboardKeySelector* KeySelector = StructProp->ContainerPtrToValuePtr<FBlackboardKeySelector>(InNode);
			if (KeySelector == nullptr)
			{
				continue;
			}
				
			const FString AccessType = ExtractBlackboardAccessType(Property->GetName(), InNode);
			const FString KeyName = KeySelector->SelectedKeyName.IsNone() ? TEXT("None") : KeySelector->SelectedKeyName.ToString();
			NodeData.CustomProperties.Add(FString::Printf(TEXT("BB_%s_%s"), *AccessType, *Property->GetName()), KeySelector->SelectedKeyName.ToString());
		}
	}

	// 상세 속성 추출 (AI 분석용 고도화)
	if (UBTDecorator* Decorator = Cast<UBTDecorator>(InNode))
	{
		NodeData.CustomProperties.Add(TEXT("InverseCondition"), Decorator->IsInversed() ? TEXT("True") : TEXT("False"));
		NodeData.CustomProperties.Add(TEXT("FlowAbortMode"), UEnum::GetValueAsString(Decorator->GetFlowAbortMode()));
	}
	// Service
	if (UBTService* Service = Cast<UBTService>(InNode))
	{
		if (const FBoolProperty* NotifyTickProp =
			FindFProperty<FBoolProperty>(
				Service->GetClass(),
				TEXT("bNotifyTick")))
		{
			const bool bNotifyTick =
				NotifyTickProp->GetPropertyValue_InContainer(Service);

			NodeData.CustomProperties.Add(
				TEXT("bNotifyTick"),
				bNotifyTick ? TEXT("True") : TEXT("False")
			);
		}

		if (const FFloatProperty* IntervalProp = FindFProperty<FFloatProperty>(Service->GetClass(), TEXT("Interval")))
		{
			const float Interval = IntervalProp->GetPropertyValue_InContainer(Service);

			NodeData.CustomProperties.Add(TEXT("Interval"), FString::SanitizeFloat(Interval));
		}

		if (const FFloatProperty* RandomDeviationProp = FindFProperty<FFloatProperty>(Service->GetClass(), TEXT("RandomDeviation")))
		{
			const float RandomDeviation = RandomDeviationProp->GetPropertyValue_InContainer(Service);

			NodeData.CustomProperties.Add(TEXT("RandomDeviation"), FString::SanitizeFloat(RandomDeviation));
		}
	}

	// Task
	if (UBTTaskNode* Task = Cast<UBTTaskNode>(InNode))
	{
		if (const FBoolProperty* NotifyTickProp =
			FindFProperty<FBoolProperty>(
				Task->GetClass(),
				TEXT("bNotifyTick")))
		{
			const bool bNotifyTick =
				NotifyTickProp->GetPropertyValue_InContainer(Task);

			NodeData.CustomProperties.Add(
				TEXT("bNotifyTick"),
				bNotifyTick ? TEXT("True") : TEXT("False")
			);
		}
	}

	if (UBTCompositeNode* CompositeNode = Cast<UBTCompositeNode>(InNode))
	{
		for (int32 Index = 0; Index < CompositeNode->Children.Num(); ++Index)
		{
			FBTCompositeChild& Child = CompositeNode->Children[Index];
			if (UBTNode* ChildNode = CompositeNode->GetChildNode(Index))
			{
				// Child node
				NodeData.ChildrenIds.Add(ProcessNode(ChildNode, OutData, NodeId, Depth + 1, Index));
			}

			for (UBTDecorator* Decorator : Child.Decorators)
			{
				if (Decorator == nullptr)
				{
					continue;
				}

				FBAAIAnalyzerDecoratorLink LinkData;
				LinkData.DecoratorId = ProcessNode(Decorator, OutData, NodeId, Depth + 1, Index);
				LinkData.ChildIndex = Index;
				NodeData.AttachedDecorators.Add(LinkData);
			}
		}

		// Decorators attached to child
		for (UBTService* Service : CompositeNode->Services)
		{
			if (Service == nullptr)
			{
				continue;
			}
			NodeData.AttachedServiceIds.Add(ProcessNode(Service, OutData, NodeId, Depth + 1, INDEX_NONE));
		}
	}

	OutData.Nodes.Add(NodeId, NodeData);

	return NodeId;
}

EBAAIAnalyzerNodeType UBABTAnalyzer::GetNodeType(UBTNode* InNode)
{
	if (InNode == nullptr)
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

EBAAICompositeLogic UBABTAnalyzer::GetCompositeLogic(UBTNode* InNode)
{
	if (InNode == nullptr)
	{
		return EBAAICompositeLogic::Unknown;
	}

	const FString ClassName = InNode->GetClass()->GetName();
	const FString LowerClassName = ClassName.ToLower();

	if (LowerClassName.Contains(TEXT("sequence")))
	{
		return EBAAICompositeLogic::Sequence;
	}

	if (LowerClassName.Contains(TEXT("selector")))
	{
		return EBAAICompositeLogic::Selector;
	}

	if (LowerClassName.Contains(TEXT("simpleparallel")))
	{
		return EBAAICompositeLogic::SimpleParallel;
	}

	return EBAAICompositeLogic::Unknown;
}

FString UBABTAnalyzer::ExtractSemanticIntent(UBTNode* InNode)
{
	if (InNode == nullptr)
	{
		return TEXT("Unknown");
	}

	const FString Name = InNode->GetClass()->GetName();
	const FString LowerName = Name.ToLower();
	if (LowerName.Contains(TEXT("moveto")))
	{
		return TEXT("Navigation");
	}

	if (LowerName.Contains(TEXT("wait")))
	{
		return TEXT("Delay");
	}

	if (LowerName.Contains(TEXT("eqs")))
	{
		return TEXT("EnvironmentQuery");
	}

	if (LowerName.Contains(TEXT("rotate")))
	{
		return TEXT("Rotation");
	}

	if (LowerName.Contains(TEXT("attack")) ||
		LowerName.Contains(TEXT("combat")) ||
		LowerName.Contains(TEXT("shoot")) ||
		LowerName.Contains(TEXT("melee")))
	{
		return TEXT("Combat");
	}

	if (LowerName.Contains(TEXT("patrol")))
	{
		return TEXT("Patrol");
	}

	return TEXT("Generic");
}

FString UBABTAnalyzer::ExtractBlackboardAccessType(const FString& PropertyName, UBTNode* Node)
{
	const FString Lower = PropertyName.ToLower();

	if (Lower.Contains(TEXT("target")))
	{
		return TEXT("Read");
	}

	if (Lower.Contains(TEXT("result")))
	{
		return TEXT("Write");
	}

	if (Lower.Contains(TEXT("check")))
	{
		return TEXT("Condition");
	}

	return TEXT("Access");
}
