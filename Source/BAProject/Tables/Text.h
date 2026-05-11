// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "Text.generated.h"

/**
 * Text.xlsx 의 "Text" 시트 한 행.
 * 필드 이름은 JSON 키와 정확히 일치해야 한다 (Text.json 참고).
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FTextRows : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	int32 TextTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FString KoreanText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FString EnglishText;

	virtual void PostRead() override
	{
		bTid = this->bTid;
	}
};