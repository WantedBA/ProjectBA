// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SkillTree/SkillNodeWidget.h"

#include "Instance/SkillTreeSubsystem.h"

void USkillNodeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	// 앵커 중앙으로 설정
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

void USkillNodeWidget::InitSkillNodeData(const FSkillTreeProgress& InitData)
{
	// TODO: 연결된 스킬 정보 초기화, ConnectionLine 자식으로 추가
	
	
}
