// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillTreeTypes.h"
#include "SaveGame/BASaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tables/BATableManager.h"
#include "SkillTreeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillNodeStateChange, int32, SkillTid, ESkillNodeState, NewState);

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
	
	// Getter
	UFUNCTION(BlueprintPure)
	[[nodiscard]]  int32 GetSkillPoints() const { return SkillPoint; }
	UFUNCTION()
	[[nodiscard]] TSet<int32> GetActivatedSkillIds() const { return ActivatedSkillIds; }

// 스킬트리 상태 반환
	bool CanLearnSkill(const int32 SkillId) const;
	
	ESkillNodeState CalculateSkillNodeState(const int32 SkillId) const;

	UFUNCTION(BlueprintCallable)
	int32 GetAvailableSkillPoints() const;

// 세이브, 로드
	FSkillTreeSaveData MakeSaveData() const;
	void ApplySaveData(const FSkillTreeSaveData& SaveData);
	
// 스킬트리 상태 변경
	UFUNCTION()
	void TryToggleSkill(int32 SkillTid);
	
	UFUNCTION()
	void TryActivateSkill(int32 SkillTid);
	
	UFUNCTION()
	void DeactivateSkill(int32 SkillTid);
	
private:
	void ActivateSkill(int32 SkillTid);
	
private:
// Subsystem
	UPROPERTY()
	TObjectPtr<class UUserDataSubsystem> UserData;
	UPROPERTY()
	TObjectPtr<UBATableManager> TableManager;

protected:
// 저장된 데이터
	// 배운 스킬 + 남은 스킬 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	int32 SkillPoint = 3;

	// 배운 스킬 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	TSet<int32> ActivatedSkillIds;
	
public:
// Delegate
	UPROPERTY(BlueprintAssignable)
	FOnSkillNodeStateChange OnSkillNodeStateChange;
	
};
