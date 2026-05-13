// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillTreeTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tables/BATableManager.h"
#include "SkillTreeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeactivateSkill, int32, NewSkillPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActivateSkill, int32, SkillTid);

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
	FORCEINLINE int32 GetSkillPoints() const { return SkillPoints; }

// 스킬트리 상태 반환
	bool CanLearnSkill(const int32 SkillId) const;

	ESkillNodeState GetSkillNodeState(const int32 SkillId) const;

	UFUNCTION(BlueprintCallable)
	int32 GetAvailableSkillPoints() const;
	
// 스킬트리 상태 변경
	UFUNCTION()
	void TryToggleSkill(int32 SkillId);
	
	UFUNCTION()
	void TryActivateSkill(int32 SkillId);
	
	UFUNCTION()
	void DeactivateSkill(int32 SkillId);
	
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
	int32 SkillPoints = 0;

	// 배운 스킬 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	TSet<int32> ActivatedSkillIds;
	
// Delegate
	UPROPERTY(BlueprintAssignable)
	FOnDeactivateSkill OnDeactivateSkill;
	UPROPERTY(BlueprintAssignable)
	FOnActivateSkill OnActivateSkill;
	
};
