// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Instance/SkillTreeTypes.h"
#include "SkillNodeWidget.generated.h"

struct FSkillTreeProgress;


/**
 * 
 */
UCLASS()
class BAPROJECT_API USkillNodeWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// 재정의 함수
	virtual void NativePreConstruct() override;

public:
	// 최초 생성 이후 1회 초기화
	void InitSkillNodeData(const FSkillTreeProgress& InitData);

// 고정 데이터
protected:
	UPROPERTY(BlueprintReadOnly, Category = SkillTree)
	int32 SkillId = -1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkillTree)
	TArray<int32> PrerequisiteSkillIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkillTree)
	TArray<int32> ChildSkillIds;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkillTree)
	// class FSkillData SkillData;

public:
	// Setter
	FORCEINLINE void SetSkillNodeState(const ESkillNodeState InSkillNodeState)
	{
		this->SkillNodeState = InSkillNodeState;
	}

// 변동 가능한 데이터
protected:
	// 게임 중 스킬 상태(SkillTree에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkillTree)
	ESkillNodeState SkillNodeState = ESkillNodeState::Locked;
	
};
