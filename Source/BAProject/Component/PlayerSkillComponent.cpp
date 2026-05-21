// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerSkillComponent.h"

#include "ActionComponent.h"
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

	// 액터컴포넌트 포인터 설정
	ActionComponent = Cast<UActionComponent>(GetOwner()->GetComponentByClass(UActionComponent::StaticClass()));
	if (!ActionComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerSkillComponent: ActionComponent not found."));
		return;
	}
	
	// 스킬트리 창이 닫힐 때 스킬 새로고침 바인딩
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
	// TODO: 임시 코드
	ActionComponent->ResetMovesetKeys();
}

void UPlayerSkillComponent::ApplySkill(int32 SkillId)
{
	const FSkillModifierRow* SkillModifier = TableManager->FindSkillModifier(SkillId);
	if (!SkillModifier)
	{
		// 기본 스킬은 SkillModifier에 없음
		UE_LOG(LogTemp, Log, TEXT("PlayerSkillComponent: SkillModifier not found for ID %d"), SkillId);
		return;
	}
	
	/*
	 * SkillId 1, 2, 3, 4, 5: 기본 스킬
	 * 6, 7, 8: CombatComponent에 NiagaraSystem 전달
	 * 9, 10, 12, 14, 20, 21: 콤보 연계 데이터 수정
	 * 11, 15, 16, 17: StatComponent에서 스탯 변경
	 * 13: ActionComponent에 MoveSetKey 추가
	 * 18, 19: 회복 아직 미구현
	 */

	switch (SkillModifier->ApplyType)
	{
	case ESkillApplyType::Action:
		ApplyAction(SkillModifier);
		break;
	case ESkillApplyType::Element:
		break;
	case ESkillApplyType::Stat:
		break;
	case ESkillApplyType::Etc:
		break;
	default:
		UE_LOG(LogTemp, Error, TEXT("PlayerSkillComponent: Invalid ApplyType for SkillId %d"), SkillId);
	}
}

void UPlayerSkillComponent::ApplyAction(const FSkillModifierRow* SkillModifier)
{
	static const FName AddMovesetKeyTarget = FName(TEXT("AddMovesetKey"));

	// ActionComponent에 관련 로직이 구현되어 있는 경우
	if (SkillModifier->Target == AddMovesetKeyTarget)
	{
		AddMovesetKey(FName(*SkillModifier->Value));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerSkillComponent: Invalid Skill Modifier Target: %s"), *SkillModifier->Target.ToString());
	}
}

void UPlayerSkillComponent::ApplyElement(const FSkillModifierRow* SkillModifier)
{
}

void UPlayerSkillComponent::ApplyStat(const FSkillModifierRow* SkillModifier)
{
}

void UPlayerSkillComponent::ApplyEtc(const FSkillModifierRow* SkillModifier)
{
}

void UPlayerSkillComponent::AddMovesetKey(const FName& NewMovesetKey)
{
	ActionComponent->AddMovesetKey(NewMovesetKey);
}

