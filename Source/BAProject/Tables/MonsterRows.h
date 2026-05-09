// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "MonsterRows.generated.h"

/**
 * Monster.xlsx 의 "Monster" 시트 한 행.
 * 필드 이름은 JSON 키와 정확히 일치해야 한다 (Monster.json 참고).
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FMonsterRows : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 MonsterTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 StageType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 NameTextTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 DescTextTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 GradeType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 MaxHp = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 Defence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 DetectRange = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 MoveSpeed = 0;

	virtual void PostRead() override
	{
		bTid = MonsterTid;
	}
};
