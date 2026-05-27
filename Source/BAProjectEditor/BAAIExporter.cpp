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

		const FString* SemanticIntent = Node.CustomProperties.Find(TEXT("SemanticIntent"));
		const FString IntentText = SemanticIntent ? *SemanticIntent : TEXT("Unknown");

		FString PropertyDetails = TEXT("");
		for (const auto& Prop : Node.CustomProperties)
		{
			// 너무 길거나 중복되는 정보는 그래프 가독성을 위해 제외
			if (Prop.Key != TEXT("StaticDescription") && Prop.Key != TEXT("SemanticIntent"))
			{
				PropertyDetails += FString::Printf(TEXT("<br/>• %s: %s"), *Prop.Key, *Prop.Value);
			}
		}

		OutContent += FString::Printf(TEXT("  %s%s\"%s\\n[%s]\"%s\n"), 
			*Node.NodeId, *ShapeStart, *Node.NodeName, *IntentText, *ShapeEnd);

		for (const FString& ChildId : Node.ChildrenIds)
		{
			OutContent += FString::Printf(TEXT("  %s --> %s\n"), *Node.NodeId, *ChildId);
		}

		for (const FBAAIAnalyzerDecoratorLink& Link : Node.AttachedDecorators)
		{
			OutContent += FString::Printf(TEXT("  %s -. \"Decorator[%d]\" .-> %s\n"), *Node.NodeId, Link.ChildIndex, *Link.DecoratorId);
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
	// D2 컴파일 에러를 방지하기 위해 트리 이름 및 ID 제어 안전화
	OutContent = FString::Printf(TEXT("\"%s\": {\n  shape: cloud\n}\n"), *InData.TreeName);

	// Blackboard 정보 추가
	if (!InData.BlackboardName.IsEmpty())
	{
		OutContent += FString::Printf(TEXT("Blackboard_%s: {\n  label: \"%s\"\n"), *InData.BlackboardName, *InData.BlackboardName);
		for (const FBAAIAnalyzerBlackboardKeyData& Key : InData.BlackboardKeys)
		{
			OutContent += FString::Printf(TEXT("  BB_%s: \"%s (%s)\"\n"), *Key.KeyName, *Key.KeyName, *Key.KeyType);
		}
		OutContent += TEXT("}\n");
	}

	for (auto& Elem : InData.Nodes)
	{
		const FBAAIAnalyzerNodeData& Node = Elem.Value;
		OutContent += FString::Printf(TEXT("\"%s\": \"%s\" {\n"), *Node.NodeId, *Node.NodeName);
		if (Node.CustomProperties.Num() > 0)
		{
			OutContent += TEXT("  Properties: {\n");
			for (const auto& Prop : Node.CustomProperties)
			{
				FString CleanValue = Prop.Value.Replace(TEXT("\""), TEXT("'")).Replace(TEXT("\n"), TEXT(" "));
				OutContent += FString::Printf(TEXT("    %s: \"%s\"\n"), *Prop.Key, *CleanValue);
			}
			OutContent += TEXT("  }\n");
		}
		OutContent += TEXT("}\n");

		for (const FString& ChildId : Node.ChildrenIds)
		{
			OutContent += FString::Printf(TEXT("\"%s\" -> \"%s\"\n"), *Node.NodeId, *ChildId);
		}

		for (const FBAAIAnalyzerDecoratorLink& Link : Node.AttachedDecorators)
		{
			OutContent += FString::Printf(TEXT("\"%s\" -> \"%s\": \"Decorator[%d]\" { style: { stroke-dash: 5 } }\n"), *Node.NodeId, *Link.DecoratorId, Link.ChildIndex);
		}

		for (const FString& SvcId : Node.AttachedServiceIds)
		{
			OutContent += FString::Printf(TEXT("\"%s\" -> \"%s\": \"Service\" { style: { stroke-dash: 3 } }\n"), *Node.NodeId, *SvcId);
		}
	}

	OutContent += FString::Printf(TEXT("\"%s\" -> \"%s\"\n"), *InData.TreeName, *InData.RootNodeId);
	return true;
}

void FBAAIExporter::SaveToFile(const FString& InFileName, const FString& InContent)
{
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("AIAnalyzer") / InFileName;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavePath), true);
	if (FFileHelper::SaveStringToFile(InContent, *SavePath))
	{
		UE_LOG(LogTemp, Log, TEXT("[BAAIExporter] Successfully saved file to: %s"), *SavePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BAAIExporter] Failed to save file to: %s"), *SavePath);
	}
}
