// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "ItemRows.generated.h"

/**
 * Item.xlsx 의 "Consume" 시트 한 행.
 * 필드 이름은 JSON 키와 정확히 일치해야 한다 (Item.json 참고).
 */
USTRUCT(BlueprintType, meta = (BASheet = "Consume"))
struct BAPROJECT_API FConsumeItemRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume")
	int32 ItemTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume")
	int32 CoolTime = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume")
	int32 NameTextTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume")
	int32 DescTextTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume")
	FString IconSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume")
	int32 Var1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume")
	int32 Var2 = 0;

	virtual void PostRead() override
	{
		bTid = ItemTid;
	}
};
