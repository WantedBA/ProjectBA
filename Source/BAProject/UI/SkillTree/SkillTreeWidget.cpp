// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeWidget.h"

#include "Instance/SkillTreeSubsystem.h"
#include "Tables/BATableManager.h"

void USkillTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 화면에 생길 때 델리게이트 등록
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->OnSkillNodeStateChange.AddUniqueDynamic(
		this, &USkillTreeWidget::HandleSkillNodeStateChanged);
}

void USkillTreeWidget::NativeDestruct()
{
	Super::NativeDestruct();
	
	// 화면에서 해제될 때 델리게이트 해제
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->OnSkillNodeStateChange.RemoveDynamic(
		this, &USkillTreeWidget::HandleSkillNodeStateChanged);
}

void USkillTreeWidget::HandleSkillNodeStateChanged(int32 SkillId, ESkillNodeState NewState)
{
	SkillNodeMap[SkillId]->SetSkillNodeState(NewState);
}

void USkillTreeWidget::RefreshAllSkillNodeState()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("USkillTreeWidget::RefreshAllSkillNodeState - GameInstance is not found"));
		return;
	}
	
	const USkillTreeSubsystem* SkillTreeSubsystem = GameInstance->GetSubsystem<USkillTreeSubsystem>();

	if (!SkillTreeSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("USkillTreeWidget::RefreshAllSkillNodeState - SkillTreeSubsystem is not found"));
		return;
	}
	
	for (auto& SkillNodePair : SkillNodeMap)
	{
		SkillNodePair.Value->SetSkillNodeState(SkillTreeSubsystem->CalculateSkillNodeState(SkillNodePair.Key));
	}
}

TArray<int32> USkillTreeWidget::GetSkillTids() const
{
	// 스킬 키 배열 생성
	TArray<int32> SkillTids;
	UBATableManager::Get(this)->GetSkillMap().GenerateKeyArray(SkillTids);
	return SkillTids;
}


void USkillTreeWidget::HandleSkillNodeClicked(int32 SkillId)
{
	// SkillTreeSubsystem으로 스킬Id 전달
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->TryToggleSkill(SkillId);
}

