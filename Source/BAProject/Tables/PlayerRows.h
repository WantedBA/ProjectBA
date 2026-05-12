// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "PlayerRows.generated.h"

/**
 * Player.xlsx 의 "BaseStat" 시트 한 행.
 * 필드 이름은 JSON 키와 정확히 일치해야 한다 (Player.json 참고).
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API FPlayerBaseStatRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 BaseHp = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 BaseStamina = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 WalkSpeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 RunSpeed = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 SprintSpeed = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 BaseAttack = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 BaseDefence = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	int32 BaseAttackSpeed = 0;

	virtual void PostRead() override
	{
		Tid = this->Tid;
	}
};

USTRUCT(BlueprintType)
struct BAPROJECT_API FPlayerActionDataRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|ActionData")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|ActionData")
	int32 ActionType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|ActionData")
	int32 ConsumeStamina = 0;

	virtual void PostRead() override
	{
		Tid = this->Tid;
	}
};