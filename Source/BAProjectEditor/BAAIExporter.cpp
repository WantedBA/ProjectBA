// Copyright TeamBA. All Rights Reserved.

#include "BAAIExporter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

bool FBAAIExporter::ExportToMermaid(const FBAAIAnalyzerTreeData& InData, FString& OutContent)
{
	OutContent = TEXT("graph TD\n");
	
	// Blackboard Info
	if (!InData.BlackboardName.IsEmpty())
	{
		OutContent += FString::Printf(TEXT("  subgraph Blackboard [%s]\n"), *InData.BlackboardName);
		for (const FBAAIAnalyzerBlackboardKeyData& Key : InData.BlackboardKeys)
		{
			OutContent += FString::Printf(TEXT("    BB_%s[\"%s (%s)\"]\n"), *Key.KeyName, *Key.KeyName, *Key.KeyType);
		}
		OutContent += TEXT("  end\n");
	}

	OutContent += FString::Printf(TEXT("  Root((%s))\n"), *InData.TreeName);

	for (auto& Elem : InData.Nodes)
	{
		const FBAAIAnalyzerNodeData& Node = Elem.Value;
		FString ShapeStart = TEXT("[");
		FString ShapeEnd = TEXT("]");

		if (Node.NodeType == EBAAIAnalyzerNodeType::Composite)
		{
			ShapeStart = TEXT("{");
			ShapeEnd = TEXT("}");
		}

		OutContent += FString::Printf(TEXT("  %s%s\"%s\"%s\n"), *Node.NodeId, *ShapeStart, *Node.NodeName, *ShapeEnd);

		for (const FString& ChildId : Node.ChildrenIds)
		{
			OutContent += FString::Printf(TEXT("  %s --> %s\n"), *Node.NodeId, *ChildId);
		}

		for (const FString& DecId : Node.AttachedDecoratorIds)
		{
			OutContent += FString::Printf(TEXT("  %s -. Decorator .-> %s\n"), *Node.NodeId, *DecId);
		}
		
		for (const FString& SvcId : Node.AttachedServiceIds)
		{
			OutContent += FString::Printf(TEXT("  %s -. Service .-> %s\n"), *Node.NodeId, *SvcId);
		}
	}

	// Link internal Root to the actual BT Root
	OutContent += FString::Printf(TEXT("  Root --> %s\n"), *InData.RootNodeId);

	return true;
}

bool FBAAIExporter::ExportToD2(const FBAAIAnalyzerTreeData& InData, FString& OutContent)
{
	OutContent = FString::Printf(TEXT("%s: {\n  shape: cloud\n}\n"), *InData.TreeName);

	// [수정] Blackboard 정보 추가 (D2 컨테이너 문법 적용)
	if (!InData.BlackboardName.IsEmpty())
	{
		// D2에서는 컨테이너명: { ... } 내부에 자식 노드들을 작성합니다.
		OutContent += FString::Printf(TEXT("Blackboard_%s: {\n  label: \"%s\"\n"), *InData.BlackboardName, *InData.BlackboardName);
		for (const FBAAIAnalyzerBlackboardKeyData& Key : InData.BlackboardKeys)
		{
			// 컨테이너 내부의 노드 정의
			OutContent += FString::Printf(TEXT("  BB_%s: \"%s (%s)\"\n"), *Key.KeyName, *Key.KeyName, *Key.KeyType);
		}
		OutContent += TEXT("}\n");
	}

	for (auto& Elem : InData.Nodes)
	{
		const FBAAIAnalyzerNodeData& Node = Elem.Value;
		OutContent += FString::Printf(TEXT("%s: \"%s\"\n"), *Node.NodeId, *Node.NodeName);

		for (const FString& ChildId : Node.ChildrenIds)
		{
			OutContent += FString::Printf(TEXT("%s -> %s\n"), *Node.NodeId, *ChildId);
		}

		for (const FString& DecId : Node.AttachedDecoratorIds)
		{
			OutContent += FString::Printf(TEXT("%s -> %s: Decorator { style: { stroke-dash: 5 } }\n"), *Node.NodeId, *DecId);
		}

		for (const FString& SvcId : Node.AttachedServiceIds)
		{
			OutContent += FString::Printf(TEXT("%s -> %s: Service { style: { stroke-dash: 3 } }\n"), *Node.NodeId, *SvcId);
		}
	}

	OutContent += FString::Printf(TEXT("%s -> %s\n"), *InData.TreeName, *InData.RootNodeId);

	return true;
}

void FBAAIExporter::SaveToFile(const FString& InFileName, const FString& InContent)
{
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("AIAnalyzer") / InFileName;
	if (FFileHelper::SaveStringToFile(InContent, *SavePath))
	{
		UE_LOG(LogTemp, Log, TEXT("[BAAIExporter] Successfully saved file to: %s"), *SavePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BAAIExporter] Failed to save file to: %s"), *SavePath);
	}
}
