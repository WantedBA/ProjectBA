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

int32 USkillTreeSubsystem::GetAvailableSkillPoints() const
{
	// TODO
	return 10;
}
