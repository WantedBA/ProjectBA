// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tables/BATableManager.h"
#include "SkillTreeSubsystem.generated.h"

// Todo: 임의 작업 후에 SkillData 관련 구조체 제대로 설정되면 변경 요망.
USTRUCT(BlueprintType)
struct FSkillTreeProgress
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	int32 SkillTid;
	UPROPERTY(BlueprintReadOnly)
	int32 SkillCost = 1; // 스킬 획득 비용(스킬포인트)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillPointsChanged, int32, NewSkillPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddSkillData, FSkillTreeProgress, SkillData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoveSkillData, int32, SkillTid);

/**
 * 
 */
UCLASS()
class BAPROJECT_API USkillTreeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// 재정의 함수
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// 전역 접근
	static USkillTreeSubsystem* Get(const UObject* WorldContext);
	
	// Getter
	UFUNCTION(BlueprintPure)
	int32 GetSkillPoints() const { return SkillPoints; }
	
private:
	// Subsystem 참조
	UPROPERTY()
	TObjectPtr<class UUserDataSubsystem> UserData;
	UPROPERTY()
	TObjectPtr<UBATableManager> TableManager;
	
	// TableManager에서 스킬 정보 가져오는 함수
	const struct FSkillRow* GetSkillRow(const int32 InTid) const { return TableManager->FindSkill(InTid); }
	

// -----------------------------------------------------------------------------------------------------
	
	
// 스킬트리 관련
	//SkillPoint
	// UFUNCTION(BlueprintCallable)
	// void AddSkillPoints(int32 Amount);

	//AddSkill
	void AddSkillData(FSkillTreeProgress data);
	void AddSkillData(int32 skillTid);
	
	//RemoveSkill
	void RemoveSkillData(int32 skillTid);
	
		
	//Delegate
	UPROPERTY(BlueprintAssignable)
	FOnSkillPointsChanged OnSkillPointsChanged;
	UPROPERTY(BlueprintAssignable)
	FOnAddSkillData OnAddSkillData;
	UPROPERTY(BlueprintAssignable)
	FOnRemoveSkillData OnRemoveSkillData;
	
	const FSkillTreeProgress* FindSkillData(int32 skillTid) const;
	
	
protected:
	UPROPERTY(BlueprintReadWrite)
	int32 SkillPoints = 0;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSkillTreeProgress> OwnedSkillDatas;
};
