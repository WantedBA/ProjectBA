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
	}
	if (!TableManager)
	{
		UE_LOG(LogTemp, Error, TEXT("BATableManager is not found in SkillTreeSubsystem Initialize"));
	}
}

bool USkillTreeSubsystem::CanLearnSkill(const int32 SkillId) const
{
	return GetSkillNodeState(SkillId) == ESkillNodeState::Available;
}

ESkillNodeState USkillTreeSubsystem::GetSkillNodeState(const int32 SkillId) const
{
	if (ActivatedSkillIds.Contains(SkillId))
	{
		return ESkillNodeState::Activated;
	}
	
	// 선행 스킬이 활성화되어 있는지 확인
	const FSkillRow* SkillRow = TableManager->FindSkill(SkillId);
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
	
	return SkillPoints - UsedSkillPoints;
}

void USkillTreeSubsystem::TryToggleSkill(int32 SkillId)
{
	ESkillNodeState SkillState = GetSkillNodeState(SkillId);
	
	if (SkillState == ESkillNodeState::Activated)
	{
		DeactivateSkill(SkillId);
	}
	else if (SkillState == ESkillNodeState::Available)
	{
		TryActivateSkill(SkillId);
	}
}

void USkillTreeSubsystem::TryActivateSkill(int32 SkillId)
{
	
}

void USkillTreeSubsystem::DeactivateSkill(int32 SkillId)
{
}
