// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SkillTree/SkillNodeWidget.h"

#include "Components/Button.h"
#include "Instance/SkillTreeSubsystem.h"

void USkillNodeWidget::SetSkillNodeState(const ESkillNodeState InSkillNodeState)
{
	SkillNodeState = InSkillNodeState;
	OnSkillNodeStateChanged();
	
	// TODO: 디버그용 임시 코드 
	FString StateString = StaticEnum<ESkillNodeState>()->GetNameStringByValue(static_cast<int64>(SkillNodeState));
	UE_LOG(LogTemp, Log, TEXT("Skill node %d state changed to %s"), SkillId, *StateString);
}

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
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SkillNodeButton is not initialized"));
	}
}

void USkillNodeWidget::HandleSkillNodeButtonClicked()
{
	OnSkillNodeClicked.Broadcast(SkillId);
}
