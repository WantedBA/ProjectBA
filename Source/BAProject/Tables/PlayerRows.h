// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tables/BAPropTable.h"
#include "Tables/PlayerEnums.h"
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
	float MaxHp = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float MaxStamina = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float StaminaRecoveryAmount = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float StaminaRecoveryDelay = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float WalkSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float RunSpeed = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float SprintSpeed = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float BaseAttack = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float BaseAttackSpeed = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|BaseStat")
	float BaseDefence = 0.f;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};

USTRUCT(BlueprintType)
struct BAPROJECT_API FPlayerActionDataRow : public FBARowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|ActionData")
	int32 Tid = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|ActionData")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|ActionData")
	EPlayerActionCategory Category = EPlayerActionCategory::Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|ActionData")
	float StaminaCost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|ActionData")
	float MinRequiredStamina = 0.f;

	virtual void PostRead() override
	{
		bTid = Tid;
	}
};
