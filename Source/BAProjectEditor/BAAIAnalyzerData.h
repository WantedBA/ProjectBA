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

UENUM(BlueprintType)
enum class EBAAICompositeLogic : uint8
{
	Unknown,
	Sequence,
	Selector,
	SimpleParallel
};

USTRUCT(BlueprintType)
struct FBAAIAnalyzerDecoratorLink
{
	GENERATED_BODY()

	UPROPERTY()
	FString DecoratorId;

	UPROPERTY()
	int32 ChildIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FBAAIAnalyzerNodeData
{
	GENERATED_BODY()

	UPROPERTY()
	FString NodeId;

	UPROPERTY()
	FString ParentNodeId;

	UPROPERTY()
	int32 Depth = 0;

	UPROPERTY()
	int32 ChildIndex = INDEX_NONE;

	UPROPERTY()
	FString NodeName;

	UPROPERTY()
	EBAAIAnalyzerNodeType NodeType;

	UPROPERTY()
	EBAAICompositeLogic CompositeLogic = EBAAICompositeLogic::Unknown;

	UPROPERTY()
	TArray<FString> ChildrenIds;

	UPROPERTY()
	TArray<FBAAIAnalyzerDecoratorLink> AttachedDecorators;

	UPROPERTY()
	TArray<FString> AttachedServiceIds;

	UPROPERTY()
	TMap<FString, FString> CustomProperties;
};

USTRUCT(BlueprintType)
struct FBAAIAnalyzerBlackboardKeyData
{
	GENERATED_BODY()

	UPROPERTY()
	FString KeyName;

	UPROPERTY()
	FString KeyType;
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
