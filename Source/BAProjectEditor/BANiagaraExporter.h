// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BANiagaraAnalyzerData.h"

class BANiagaraExporter
{
public:
	static bool ExportToText(const FNiagaraGameplayAnalysisData& InData, FString& OutContent);
	static bool ExportToJson(const FNiagaraGameplayAnalysisData& InData, FString& OutContent);
	static void SaveToFile(const FString& InFileName, const FString& InContent);
};
