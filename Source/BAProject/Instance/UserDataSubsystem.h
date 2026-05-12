// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SubSystems/GameInstanceSubsystem.h"
#include "Tables/BATableManager.h"
#include "UserDataSubsystem.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FPlayerBaseStat
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	int32 MaxHp = 0; // BaseHp
	UPROPERTY(BlueprintReadWrite)
	int32 MaxStamina = 0; // BaseStamina
	UPROPERTY(BlueprintReadWrite)
	int32 WalkSpeed = 0; // WalkSpeed
	UPROPERTY(BlueprintReadWrite)
	int32 RunSpeed = 0; // RunSpeed
	UPROPERTY(BlueprintReadWrite)
	int32 SprintSpeed = 0; // SprintSpeed
	UPROPERTY(BlueprintReadWrite)
	int32 BaseAttack = 0; // BaseAttack
	UPROPERTY(BlueprintReadWrite)
	int32 BaseAttackSpeed = 0; // BaseAttackSpeed
	UPROPERTY(BlueprintReadWrite)
	int32 BaseDefence = 0; // BaseDefence
};

UCLASS()
class BAPROJECT_API UUserDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UUserDataSubsystem();
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	static UUserDataSubsystem* Get(const UObject* WorldContext);
	
	void SetBaseStat();

protected:
	// 초기스탯
	UPROPERTY(BlueprintReadOnly)
	FPlayerBaseStat BaseStat;
	
	// 게임 런타임용 변수
	UPROPERTY(BlueprintReadWrite)
	int32 CurMaxHp = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 CurMaxStamina = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 CurMoveSpeed = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 CurAttack = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 CurAttackSpeed = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 CurDefence = 0;
	
};
