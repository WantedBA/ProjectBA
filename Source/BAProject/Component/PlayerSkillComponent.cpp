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
	
	// 서브시스템 포인터 설정
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			SkillTreeSubsystem = GameInstance->GetSubsystem<USkillTreeSubsystem>();
			TableManager = UBATableManager::Get(this);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerSkillComponent: GameInstance not found."));
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerSkillComponent: World not found."));
		return;
	}
	
	// 스킬트리 창이 닫힐 때(TODO) 스킬 새로고침 바인딩 추가
	SkillTreeSubsystem->OnSkillTreeChangeCompleted.AddUniqueDynamic(this, &UPlayerSkillComponent::RefreshAllSkills);
}


// Called every frame
void UPlayerSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPlayerSkillComponent::RefreshAllSkills()
{
	// 기존 스킬 변경사항 초기화
	ClearAllSkills();
	
	// 스킬트리 서브시스템에서 활성화된 스킬 ID 들을 가져와 적용
	TArray<int32> ActivatedSkillIds = SkillTreeSubsystem->GetActivatedSkillIds().Array();
	ActivatedSkillIds.Sort();
	for (const int32 SkillId : ActivatedSkillIds)
	{
		ApplySkill(SkillId);
	}
}

void UPlayerSkillComponent::ClearAllSkills()
{
}

void UPlayerSkillComponent::ApplySkill(int32 SkillId)
{
}

