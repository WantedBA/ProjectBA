// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "BossMonster.generated.h"

/**
 * BossMonster.xlsx 의 각 시트.
 */
USTRUCT(BlueprintType)
struct BAPROJECT_API F1StageBossAttackRows : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 CoolTime = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 ConditionType = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 Var1 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 Var2 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 Var3 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1StageBossAttack")
	int32 Attack = 0;

	virtual void PostRead() override
	{
		Tid = this->Tid;
	}
};

USTRUCT(BlueprintType)
struct BAPROJECT_API F2StageBossAttackRows : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 CoolTime = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 ConditionType = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 Var1 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 Var2 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 Var3 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2StageBossAttack")
	int32 Attack = 0;

	virtual void PostRead() override
	{
		Tid = this->Tid;
	}
};

USTRUCT(BlueprintType)
struct BAPROJECT_API F3StageBossAttackRows : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 CoolTime = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 ConditionType = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 Var1 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 Var2 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 Var3 = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3StageBossAttack")
	int32 Attack = 0;

	virtual void PostRead() override
	{
		Tid = this->Tid;
	}
};