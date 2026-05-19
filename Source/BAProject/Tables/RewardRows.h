// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "RewardRows.generated.h"

USTRUCT(BlueprintType, meta = (BASheet = "Reward"))
struct BAPROJECT_API FRewardRows : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 RewardDetailTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 RewardTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 RewardType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 TargetTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 Count = 0;

	virtual void PostRead() override
	{
		bTid = RewardDetailTid;
	}
};
