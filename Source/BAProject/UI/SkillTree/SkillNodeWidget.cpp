// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SkillTree/SkillNodeWidget.h"

#include "Components/Button.h"
#include "Instance/SkillTreeSubsystem.h"

void USkillNodeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	// 앵커 중앙으로 설정
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

void USkillNodeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (SkillNodeButton)
	{
		SkillNodeButton->OnClicked.AddDynamic(this, &USkillNodeWidget::HandleSkillNodeButtonClicked);
	}
}

void USkillNodeWidget::HandleSkillNodeButtonClicked()
{
	OnSkillNodeClicked.Broadcast(SkillId);
}
