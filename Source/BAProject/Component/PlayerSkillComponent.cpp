// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerSkillComponent.h"

#include "Instance/SkillTreeSubsystem.h"

// Sets default values for this component's properties
UPlayerSkillComponent::UPlayerSkillComponent()
{
	// PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UPlayerSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 스킬트리서브시스템 포인터 설정
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			SkillTreeSubsystem = GameInstance->GetSubsystem<USkillTreeSubsystem>();
		}
	}
	
	// 스킬트리 창이 닫힐 때(TODO) 스킬 새로고침 
	SkillTreeSubsystem->OnSkillTreeChanged.AddUniqueDynamic(this, &UPlayerSkillComponent::RefreshAllSkills);
}


// Called every frame
void UPlayerSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

