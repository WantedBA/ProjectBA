// Copyright TeamBA. All Rights Reserved.

#include "UserDataSubsystem.h"

UUserDataSubsystem::UUserDataSubsystem()
{
}

void UUserDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	//SubSystem간 순서보장을 위한 의존성 설정 
	Collection.InitializeDependency<UBATableManager>(); 
	
	Super::Initialize(Collection);
	
	//기본 스탯 설정
	SetBaseStat();
}

void UUserDataSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UUserDataSubsystem* UUserDataSubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext) return nullptr;

	const UWorld* World = WorldContext->GetWorld();
	if (!World) return nullptr;

	const UGameInstance* GI = World->GetGameInstance();
	if (!GI) return nullptr;

	return GI->GetSubsystem<UUserDataSubsystem>();
}

void UUserDataSubsystem::SetBaseStat()
{
	const UBATableManager* TableManager = GetGameInstance()->GetSubsystem<UBATableManager>();
	if (!TableManager)
	{
		return;
	}

	// BaseStat 멤버변수에 주입
	BaseStat.MaxHp = TableManager->FindPlayerBaseStat()->BaseHp;
	BaseStat.MaxStamina = TableManager->FindPlayerBaseStat()->BaseStamina;
	BaseStat.WalkSpeed = TableManager->FindPlayerBaseStat()->WalkSpeed;
	BaseStat.RunSpeed = TableManager->FindPlayerBaseStat()->RunSpeed;
	BaseStat.SprintSpeed = TableManager->FindPlayerBaseStat()->SprintSpeed;
	BaseStat.BaseAttack = TableManager->FindPlayerBaseStat()->BaseAttack;
	BaseStat.BaseAttackSpeed = TableManager->FindPlayerBaseStat()->BaseAttackSpeed;
	BaseStat.BaseDefence = TableManager->FindPlayerBaseStat()->BaseDefence;
	
	// 게임 런타임용 변수 초기화
	// TODO: 런타임용 변수 변경용 함수 분리
	CurMaxHp = BaseStat.MaxHp;
	CurMaxStamina  = BaseStat.MaxStamina;
	CurMoveSpeed  = BaseStat.RunSpeed;
	CurAttack  = BaseStat.BaseAttack;
	CurAttackSpeed  = BaseStat.BaseAttackSpeed;
	CurDefence  = BaseStat.BaseDefence;
}
