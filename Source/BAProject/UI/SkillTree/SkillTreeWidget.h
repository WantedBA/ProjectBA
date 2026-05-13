// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillNodeWidget.h"
#include "UI/System/PopupBase.h"
#include "SkillTreeWidget.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API USkillTreeWidget : public UPopupBase
{
	GENERATED_BODY()
	
protected:
	// SkillNodeWidget의 델리게이트로 실행됨
	UFUNCTION(BlueprintCallable, Category=SkillTree)
	void HandleSkillNodeClicked(int32 SkillId);

	
	UFUNCTION(BlueprintCallable, Category=SkillTree)
	TArray<int32> GetSkillTids() const;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USkillNodeWidget> SkillNodeWidgetClass;
	
// 데이터
	// 스킬 Tid를 키 값으로 하는 스킬 노드 맵
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	TMap<int32, TObjectPtr<USkillNodeWidget>> SkillNodeMap;
	
};
