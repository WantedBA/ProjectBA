// Copyright TeamBA. All Rights Reserved.

#include "UserDataSubSystem.h"

UUserDataSubSystem::UUserDataSubSystem()
{
}

void UUserDataSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	//SubSystem간 순서보장을 위한 의존성 설정 
	Collection.InitializeDependency<UBATableManager>(); 
	
	Super::Initialize(Collection);
	
	//기본 스탯 설정
	SetBaseStat();
}

void UUserDataSubSystem::Deinitialize()
{
	Super::Deinitialize();
}

UUserDataSubSystem* UUserDataSubSystem::Get(const UObject* WorldContext)
{
	if (!WorldContext) return nullptr;

	UWorld* World = WorldContext->GetWorld();
	if (!World) return nullptr;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return nullptr;

	return GI->GetSubsystem<UUserDataSubSystem>();
}

void UUserDataSubSystem::SetBaseStat()
{
	//BaseStat.MaxHp = // 테이블에서 온 데이터 추가
	
	// 아래는 TableManager 호출 예시
	UBATableManager* TableManager = GetGameInstance()->GetSubsystem<UBATableManager>();
	if (!TableManager) return;
	
	//BaseStat은 TableManager에서 가져온 값으로 설정
	
	CurMaxHp = BaseStat.MaxHp;
	CurMaxStamina  = BaseStat.MaxStamina;
}

void UUserDataSubSystem::AddSkillPoints(int32 Amount)
{
	// 스킬 사용시 빼는 작업 필요
	if (Amount != 0)
	{
		SkillPoints += Amount;
		OnSkillPointsChanged.Broadcast(SkillPoints);
	}
}

void UUserDataSubSystem::AddSkillData(FSkillData data)
{
	//Todo: 스킬이 체력 또는 스태미너에 영향을 미치는 경우 여기서 수정.
	
	AddSkillPoints(-data.SkillCost);
	OwnedSkillDatas.Add(data);
	OnAddSkillData.Broadcast(data);
}

void UUserDataSubSystem::AddSkillData(int32 skillTid)
{
	//Todo: 위 함수의 오버로딩 2중 한개가 메인이 되게 작업 필요.
	
	if (FindSkillData(skillTid))
	{
		//이미 가지고 있는 상태.
	}
	else
	{
		FSkillData SkillData;
		SkillData.SkillTid = skillTid;
		SkillData.SkillCost = 1; //Todo: Table에서 가져오는 값을 사용해야함 일단은 임시값 1
		
		AddSkillData(SkillData);
	}
}

void UUserDataSubSystem::RemoveSkillData(int32 skillTid)
{
	//RemoveAll은 없으면 0 반환하니 그냥 사용
	int32 removeCount = OwnedSkillDatas.RemoveAll([skillTid](const FSkillData& Skill)
	{
		return Skill.SkillTid == skillTid;
	});
	
	if (removeCount > 0)
	{
		OnRemoveSkillData.Broadcast(skillTid);
	}
}

const FSkillData* UUserDataSubSystem::FindSkillData(int32 skillTid) const
{
	return OwnedSkillDatas.FindByPredicate([skillTid](const FSkillData& Skill)
	{
		return skillTid == Skill.SkillTid;
	});
}
