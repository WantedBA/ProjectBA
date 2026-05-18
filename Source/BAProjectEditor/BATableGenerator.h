// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FJsonValue;
struct FBASheetSpec;

/**
 * 메뉴의 "GenerateTableData" 항목을 구동:
 *   1) ExcelToJsonConverter.exe 실행 (.xlsx 가 없으면 사실상 no-op),
 *   2) BADesign/Json/*.json 을 읽고,
 *   3) /Game/Table 아래에 DataTable 을 빌드 (에셋마다 클린 재빌드).
 *
 * 에디터 전용 — 패키지 빌드에는 컴파일되지 않는다.
 */
class FBATableGenerator
{
public:
	static void Generate();
	static bool GenerateHeadless();

private:
	static bool GenerateInternal(bool bShowUi);
	static bool RunConverterExe(FString& OutLog);
	static void ImportAllJson();
	static void ImportJsonFile(const FString& JsonPath);
	static bool BuildAndSaveDataTable(
		const FString& AssetName,
		const FString& PackagePath,
		const FBASheetSpec& Spec,
		const TArray<TSharedPtr<FJsonValue>>& JsonRows,
		FString& OutError);
};
