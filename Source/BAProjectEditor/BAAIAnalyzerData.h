// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BAAIAnalyzerData.generated.h"

UENUM(BlueprintType)
enum class EBAAIAnalyzerNodeType : uint8
{
	Unknown     UMETA(DisplayName = "Unknown"),
	Composite   UMETA(DisplayName = "Composite"),
	Task        UMETA(DisplayName = "Task"),
	Decorator   UMETA(DisplayName = "Decorator"),
	Service     UMETA(DisplayName = "Service")
};

USTRUCT(BlueprintType)
struct FBAAIAnalyzerNodeData
{
	GENERATED_BODY()

	UPROPERTY()
	FString NodeId;

	UPROPERTY()
	FString NodeName;

	UPROPERTY()
	EBAAIAnalyzerNodeType NodeType;

	UPROPERTY()
	TArray<FString> ChildrenIds;

	UPROPERTY()
	TArray<FString> AttachedDecoratorIds;

	UPROPERTY()
	TArray<FString> AttachedServiceIds;

	FBAAIAnalyzerNodeData()
		: NodeType(EBAAIAnalyzerNodeType::Task)
	{
	}
};

USTRUCT(BlueprintType)
struct FBAAIAnalyzerBlackboardKeyData
{
	GENERATED_BODY()

	UPROPERTY()
	FString KeyName;

	UPROPERTY()
	FString KeyType;

	FBAAIAnalyzerBlackboardKeyData() {}
};

USTRUCT(BlueprintType)
struct FBAAIAnalyzerTreeData
{
	GENERATED_BODY()

	UPROPERTY()
	FString TreeName;

	UPROPERTY()
	FString BlackboardName;

	UPROPERTY()
	TArray<FBAAIAnalyzerBlackboardKeyData> BlackboardKeys;

	UPROPERTY()
	TMap<FString, FBAAIAnalyzerNodeData> Nodes;

	UPROPERTY()
	FString RootNodeId;
};
