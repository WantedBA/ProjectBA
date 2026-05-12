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
	int32 MaxHp = 0; // 테이블에서 정의한 HP 최대치
	UPROPERTY(BlueprintReadWrite)
	int32 MaxStamina = 0; // 테이블에서 정의한 Stamina 최대치
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
	
	//BaseStat 기본값
	void SetBaseStat();
	
	//Getter
	UFUNCTION(BlueprintPure)
	int32 GetCurMaxStamina() const { return CurMaxStamina; }

protected:
	UPROPERTY(BlueprintReadOnly)
	FPlayerBaseStat BaseStat;
	UPROPERTY(BlueprintReadWrite)
	int32 CurMaxHp = 0; // 스킬에 따라 더 늘어날 수 있음
	UPROPERTY(BlueprintReadWrite)
	int32 CurMaxStamina = 0; // 스킬에 따라 더 늘어날 수 있음
};
