// Fill out your copyright notice in the Description page of Project Settings.


#include "Instance/SkillTreeSubsystem.h"

#include "UserDataSubSystem.h"
#include "Tables/BATableManager.h"

void USkillTreeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// 초기화 순서 보장
	Collection.InitializeDependency<UUserDataSubSystem>();
	
	Super::Initialize(Collection);
	
	UserData = GetGameInstance()->GetSubsystem<UUserDataSubSystem>();
	TableManager = GetGameInstance()->GetSubsystem<UBATableManager>();
}
