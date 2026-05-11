// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "PatrolPathRow.generated.h"

/**
 * PatrolPath.xlsx 의 "PatrolPath" 시트 한 행.
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FPatrolPathRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FName PathTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TArray<FVector> Points;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};
