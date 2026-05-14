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
	const USkillTreeSubsystem* SkillTreeSubsystem = GetGameInstance()->GetSubsystem<USkillTreeSubsystem>();

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

void USkillTreeWidget::HandleSkillNodeClicked(int32 SkillId)
{
	// SkillTreeSubsystem으로 스킬Id 전달
	GetGameInstance()->GetSubsystem<USkillTreeSubsystem>()->TryToggleSkill(SkillId);
}

TArray<int32> USkillTreeWidget::GetSkillTids() const
{
	// Table Manager에서 Map을 가져와 키 배열 생성
	// TODO: WBP 생성 로직 cpp로 이동(현재 블루프린트에 구현되어 있음)
	TArray<int32> SkillTids;
	TMap<int32, FSkillRow*> SkillMap = GetGameInstance()->GetSubsystem<UBATableManager>()->GetSkillMap();
	SkillMap.GenerateKeyArray(SkillTids);
	return SkillTids;
}
