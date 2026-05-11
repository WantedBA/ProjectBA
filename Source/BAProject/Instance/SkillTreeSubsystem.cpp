// Fill out your copyright notice in the Description page of Project Settings.


#include "Instance/SkillTreeSubsystem.h"

#include "UserDataSubSystem.h"

void USkillTreeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// 초기화 순서 보장
	Collection.InitializeDependency<UUserDataSubSystem>();
	
	UserData = GetGameInstance()->GetSubsystem<UUserDataSubSystem>();
}
