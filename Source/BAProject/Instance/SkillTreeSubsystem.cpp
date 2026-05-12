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
}
