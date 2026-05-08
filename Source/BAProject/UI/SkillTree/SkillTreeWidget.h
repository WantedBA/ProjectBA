// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillNodeWidget.h"
#include "Blueprint/UserWidget.h"
#include "SkillTreeWidget.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API USkillTreeWidget : public UUserWidget
{
	GENERATED_BODY()
	
// 스킬트리 최초 구성
public:
	UFUNCTION(BlueprintCallable, Category=SkillTree)
	void InitSkillTree();

protected:
	// 스킬 노드 생성
	USkillNodeWidget* CreateSkillNodeWidget(const FSkillData& SkillData);
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USkillNodeWidget> SkillNodeWidgetClass;

	UPROPERTY()
	TMap<int32, TObjectPtr<USkillNodeWidget>> SkillNodeMap;
	
// 스킬트리 창 안에서 사용
public:
	// 스킬 노드 상태 갱신
	void RefreshAllNodeStates();

	// 현재 스킬 상태
	bool IsSkillLearned(int32 SkillId);
	bool CanLearnSkill(int32 SkillId);
	
	// 스킬 노드 클릭 이벤트 처리
	void OnSkillNodeClicked(int32 SkillId);
	
	// 스킬 포인트 증감
	void AddSkillPoint(int32 Amount);
	void SpendSkillPoint(int32 Amount);
	
protected:
	// 전체 스킬 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	int32 SkillPoint = 0;
};
