// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTreeWidget.h"

#include "Instance/SkillTreeSubsystem.h"
#include "Tables/BATableManager.h"

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
