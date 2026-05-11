// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SubSystems/GameInstanceSubsystem.h"
#include "Tables/BATableManager.h"
#include "UserDataSubSystem.generated.h"

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

// Todo: 임의 작업 후에 SkillData 관련 구조체 제대로 설정되면 변경 요망.
USTRUCT(BlueprintType)
struct FSkillData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	int32 SkillTid;
	UPROPERTY(BlueprintReadOnly)
	int32 SkillCost = 1; // 스킬 획득 비용(스킬포인트)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillPointsChanged, int32, NewSkillPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddSkillData, FSkillData, SkillData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoveSkillData, int32, SkillTid);

UCLASS()
class BAPROJECT_API UUserDataSubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UUserDataSubSystem();
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	static UUserDataSubSystem* Get(const UObject* WorldContext);
	
	//BaseStat 기본값
	void SetBaseStat();
	
// 스킬트리 관련
	//SkillPoint
	UFUNCTION(BlueprintCallable)
	void AddSkillPoints(int32 Amount);

	//AddSkill
	void AddSkillData(FSkillData data);
	void AddSkillData(int32 skillTid);
	
	//RemoveSkill
	void RemoveSkillData(int32 skillTid);
	
	//Getter
	UFUNCTION(BlueprintPure)
	int32 GetCurMaxStamina() const { return CurMaxStamina; }
	UFUNCTION(BlueprintPure)
	int32 GetSkillPoints() const { return SkillPoints; }
	
	//Delegate
	UPROPERTY(BlueprintAssignable)
	FOnSkillPointsChanged OnSkillPointsChanged;
	UPROPERTY(BlueprintAssignable)
	FOnAddSkillData OnAddSkillData;
	UPROPERTY(BlueprintAssignable)
	FOnRemoveSkillData OnRemoveSkillData;
	
	const FSkillData* FindSkillData(int32 skillTid) const;
	
protected:
	UPROPERTY(BlueprintReadOnly)
	FPlayerBaseStat BaseStat;
	UPROPERTY(BlueprintReadWrite)
	int32 SkillPoints = 0;
	UPROPERTY(BlueprintReadWrite)
	int32 CurMaxHp = 0; // 스킬에 따라 더 늘어날 수 있음
	UPROPERTY(BlueprintReadWrite)
	int32 CurMaxStamina = 0; // 스킬에 따라 더 늘어날 수 있음
	UPROPERTY(BlueprintReadOnly)
	TArray<FSkillData> OwnedSkillDatas;
	
};
