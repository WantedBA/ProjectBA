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
	
	// 전역 접근
	static USkillTreeSubsystem* Get(const UObject* WorldContext)
	{
		if (!WorldContext) return nullptr;

		const UWorld* World = WorldContext->GetWorld();
		if (!World) return nullptr;

		const UGameInstance* GI = World->GetGameInstance();
		if (!GI) return nullptr;

		return GI->GetSubsystem<USkillTreeSubsystem>();
	}
	
	// Getter
	UFUNCTION(BlueprintPure)
	FORCEINLINE int32 GetSkillPoints() const { return SkillPoints; }

// 스킬트리 상태 반환
	bool CanLearnSkill(const int32 SkillId) const;

	ESkillNodeState GetSkillNodeState(const int32 SkillId) const;

	UFUNCTION(BlueprintCallable)
	int32 GetAvailableSkillPoints() const;
	
// 스킬트리 상태 변경
	void TryDeactivateSkill(const int32 SkillId);
	
private:
	// Subsystem 참조
	UPROPERTY()
	TObjectPtr<class UUserDataSubsystem> UserData;
	UPROPERTY()
	TObjectPtr<UBATableManager> TableManager;
	
	// TableManager에서 스킬 정보 가져오는 함수
	FORCEINLINE const struct FSkillRow* GetSkillRow(const int32 InTid) const { return TableManager->FindSkill(InTid); }

// 저장된 데이터
protected:
	// 배운 스킬 + 남은 스킬 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	int32 SkillPoints = 0;

	// 배운 스킬 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	TSet<int32> LearnedSkillIds;
	
	// Delegate
	UPROPERTY(BlueprintAssignable)
	FOnDeactivateSkill OnDeactivateSkill;
	UPROPERTY(BlueprintAssignable)
	FOnActivateSkill OnActivateSkill;
	
};
