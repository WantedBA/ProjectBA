// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FBATableGenerator 가 JSON 을 UDataTable 로 변환할 때 참조하는 시트별 메타데이터.
 *
 *   RowStruct     - 잎(leaf) USTRUCT (FBARowBase 를 반드시 상속).
 *   KeyColumnName - FName 행 ID 로 쓸 JSON 필드명. 행의 PostRead() 가
 *                   FBARowBase::Tid 로 매핑하는 컬럼이기도 함.
 */
struct FBASheetSpec
{
	UScriptStruct* RowStruct = nullptr;
	FString KeyColumnName;
};

class FBASheetSpecs
{
public:
	/** 시트명에 대한 spec 반환. 등록되지 않은 시트면 nullptr. */
	static const FBASheetSpec* Find(const FString& SheetName);

	/** 등록된 모든 시트명 반환. */
	static TArray<FString> GetAllSheetNames();
};
