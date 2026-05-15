// Fill out your copyright notice in the Description page of Project Settings.


#include "Instance/SkillTreeSubsystem.h"

#include "UserDataSubsystem.h"
#include "Tables/BATableManager.h"

void USkillTreeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// 초기화 순서 보장
	Collection.InitializeDependency<UUserDataSubsystem>();
	
	Super::Initialize(Collection);
	
	UserData = GetGameInstance()->GetSubsystem<UUserDataSubsystem>();
	TableManager = GetGameInstance()->GetSubsystem<UBATableManager>();
	
	if (!UserData)
	{
		UE_LOG(LogTemp, Error, TEXT("UserDataSubsystem is not found in SkillTreeSubsystem Initialize"));
		return;
	}
	if (!TableManager)
	{
		UE_LOG(LogTemp, Error, TEXT("BATableManager is not found in SkillTreeSubsystem Initialize"));
		return;
	}
	
	TArray<FSkillRow*> SkillRows;
	TableManager->GetSkillMap().GenerateValueArray(SkillRows);
	for (FSkillRow* SkillRow : SkillRows)
	{
		if (SkillRow->bIsDefaultSkill)
		{
			ActivateSkill(SkillRow->SkillTid);
		}
	}
}

bool USkillTreeSubsystem::CanLearnSkill(const int32 SkillId) const
{
	return CalculateSkillNodeState(SkillId) == ESkillNodeState::Available;
}

ESkillNodeState USkillTreeSubsystem::CalculateSkillNodeState(const int32 SkillId) const
{
	if (ActivatedSkillIds.Contains(SkillId))
	{
		return ESkillNodeState::Activated;
	}
	
	// 선행 스킬이 활성화되어 있는지 확인
	const FSkillRow* SkillRow = TableManager->FindSkill(SkillId);
	if (!SkillRow)
	{
		UE_LOG(LogTemp, Error, TEXT("SkillRow is not found in CalculateSkillNodeState"));
		return ESkillNodeState::Locked;
	}
	for (int32 PrerequisiteId : SkillRow->PrerequisiteIds)
	{
		if (!ActivatedSkillIds.Contains(PrerequisiteId))
		{
			return ESkillNodeState::Locked;
		}
	}
	
	return ESkillNodeState::Available;
}

int32 USkillTreeSubsystem::GetAvailableSkillPoints() const
{
	int32 UsedSkillPoints = 0;
	
	for (int32 ActivatedSkillId : ActivatedSkillIds)
	{
		UsedSkillPoints += TableManager->FindSkill(ActivatedSkillId)->NeededSkillPoint;
	}
	
	return SkillPoint - UsedSkillPoints;
}

FSkillTreeSaveData USkillTreeSubsystem::MakeSaveData() const
{
	return { SkillPoint, ActivatedSkillIds };
}

void USkillTreeSubsystem::ApplySaveData(const FSkillTreeSaveData& SaveData)
{
	SkillPoint = SaveData.SkillPoint;
	ActivatedSkillIds = SaveData.ActivatedSkillIds;
}

void USkillTreeSubsystem::TryToggleSkill(int32 SkillTid)
{
	ESkillNodeState SkillState = CalculateSkillNodeState(SkillTid);
	
	if (SkillState == ESkillNodeState::Activated)
	{
		DeactivateSkill(SkillTid);
	}
	else if (SkillState == ESkillNodeState::Available)
	{
		TryActivateSkill(SkillTid);
	}
	else if (SkillState == ESkillNodeState::Locked)
	{
		UE_LOG(LogTemp, Log, TEXT("Skill %d is locked and cannot be activated"), SkillTid);
		return;
	}
}

void USkillTreeSubsystem::TryActivateSkill(int32 SkillTid)
{
	// 잔여 스킬 포인트 확인
	if (GetAvailableSkillPoints() < TableManager->FindSkill(SkillTid)->NeededSkillPoint)
	{
		UE_LOG(LogTemp, Log, 
			TEXT("Not enough skill points to activate skill %d - requires: %d, available: %d"), 
			SkillTid, TableManager->FindSkill(SkillTid)->NeededSkillPoint, GetAvailableSkillPoints());
		return;
	}
	
	ActivateSkill(SkillTid);
}

void USkillTreeSubsystem::DeactivateSkill(int32 SkillTid)
{
	// 기본 스킬 확인
	if (TableManager->FindSkill(SkillTid)->bIsDefaultSkill)
	{
		UE_LOG(LogTemp, Log, TEXT("Cannot deactivate default skill %d"), SkillTid);
		return;		
	}
	
	// 재귀 종료 조건
	if (!ActivatedSkillIds.Contains(SkillTid))
	{
		OnSkillNodeStateChange.Broadcast(SkillTid, CalculateSkillNodeState(SkillTid));
		return;
	}
	
	// 스킬 비활성화
	ActivatedSkillIds.Remove(SkillTid);
	OnSkillNodeStateChange.Broadcast(SkillTid, CalculateSkillNodeState(SkillTid));
	UE_LOG(LogTemp, Log, TEXT("Skill %d deactivated"), SkillTid);
	
	// 자식 노드 상태 갱신
	for (int32 ChildId : TableManager->FindSkill(SkillTid)->ChildIds)
	{
		DeactivateSkill(ChildId);
	}
}

void USkillTreeSubsystem::ActivateSkill(int32 SkillTid)
{
	// 현재 노드 상태 변경
	ActivatedSkillIds.Add(SkillTid);
	OnSkillNodeStateChange.Broadcast(SkillTid, ESkillNodeState::Activated);
	UE_LOG(LogTemp, Log, TEXT("Skill %d activated"), SkillTid);
	
	// 자식 노드 상태 확인
	for (int32 ChildId : TableManager->FindSkill(SkillTid)->ChildIds)
	{
		if (CalculateSkillNodeState(ChildId) == ESkillNodeState::Available)
		{
			OnSkillNodeStateChange.Broadcast(ChildId, ESkillNodeState::Available);
		}
	}
	
	// 상호 배타 그룹 적용
	if (!TableManager->FindSkill(SkillTid)->ExclusiveSkills.IsEmpty())
	{
		for (int32 ExclusiveSkillId : TableManager->FindSkill(SkillTid)->ExclusiveSkillIds)
		{
			DeactivateSkill(ExclusiveSkillId);
		}
	}
}
