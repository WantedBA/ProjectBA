// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BAAIAnalyzerData.h"

class BAPROJECTEDITOR_API FBAAIExporter
{
public:
	static bool ExportToMermaid(const FBAAIAnalyzerTreeData& InData, FString& OutContent);
	static bool ExportToD2(const FBAAIAnalyzerTreeData& InData, FString& OutContent);
	static void SaveToFile(const FString& InFileName, const FString& InContent);
};
