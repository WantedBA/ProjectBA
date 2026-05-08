// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Instance/UserDataSubSystem.h"
#include "SkillNodeWidget.generated.h"

// 게임 중 스킬 상태
UENUM(BlueprintType)
enum class ESkillNodeState : uint8
{
	Locked		UMETA(DisplayName = "Locked"),
	Available	UMETA(DisplayName = "Available"),
	Learned		UMETA(DisplayName = "Learned"),
};

/**
 * 
 */
UCLASS()
class BAPROJECT_API USkillNodeWidget : public UUserWidget
{
	GENERATED_BODY()
	
	
// 스킬트리 연결 데이터(최초 초기화 1회)
public:
	// 최초 생성 시 초기화
	void InitSkillNodeData(const FSkillData& InitData);
	
	FORCEINLINE [[nodiscard]] int32 GetSkillId() const
	{
		return SkillId;
	}

	FORCEINLINE [[nodiscard]] TArray<int32> GetPrerequisiteSkillIds() const
	{
		return PrerequisiteSkillIds;
	}

	FORCEINLINE [[nodiscard]] TArray<int32> GetChildSkillIds() const
	{
		return ChildSkillIds;
	}

protected:
	// ChildSkill 설정.
	void SetChildSkillIds(const TArray<int32>& InChildSkillIds)
	{
		this->ChildSkillIds = InChildSkillIds;
	}
	
	UPROPERTY(BlueprintReadOnly, Category = SkillTree)
	int32 SkillId = -1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkillTree)
	TArray<int32> PrerequisiteSkillIds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkillTree)
	TArray<int32> ChildSkillIds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkillTree)
	FSkillData SkillData;

// 게임 중 상태
public:
	FORCEINLINE void SetSkillNodeState(const ESkillNodeState InSkillNodeState)
	{
		this->SkillNodeState = InSkillNodeState;
	}

protected:
	// 게임 중 스킬 상태(SkillTree에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkillTree)
	ESkillNodeState SkillNodeState = ESkillNodeState::Locked;
	
};
