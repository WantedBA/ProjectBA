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
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USkillNodeWidget> SkillNodeWidgetClass;

	// 스킬 Tid를 키 값으로 하는 스킬 노드 맵
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillTree)
	TMap<int32, TObjectPtr<USkillNodeWidget>> SkillNodeMap;
	
};
