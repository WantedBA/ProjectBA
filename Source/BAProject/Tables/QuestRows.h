// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "QuestRows.generated.h"

USTRUCT(BlueprintType, meta = (BASheet = "Quest"))
struct BAPROJECT_API FQuestRows : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 QuestTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 QuestType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 NameTextTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 DescTextTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 RewardTid = 0;

	virtual void PostRead() override
	{
		bTid = QuestTid;
	}
};

USTRUCT(BlueprintType, meta = (BASheet = "ZoneMonster"))
struct BAPROJECT_API FZoneMonsterRows : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 ZoneMonsterTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 QuestTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 MonsterTid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 SpawnCount = 0;

	virtual void PostRead() override
	{
		bTid = ZoneMonsterTid;
	}
};
