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
		UE_LOG(LogTemp, Error, TEXT("[UserDataSubsystem] TableManager subsystem is null."));
		return;
	}

	const FPlayerBaseStatRow* BaseStatRow = TableManager->FindPlayerBaseStat();
	if (!BaseStatRow)
	{
		UE_LOG(LogTemp, Error, TEXT("[UserDataSubsystem] Player BaseStat row(Tid=1) not found."));
		return;
	}

	// BaseStat 멤버변수에 주입
	BaseStat.MaxHp = BaseStatRow->BaseHp;
	BaseStat.MaxStamina = BaseStatRow->BaseStamina;
	BaseStat.WalkSpeed = BaseStatRow->WalkSpeed;
	BaseStat.RunSpeed = BaseStatRow->RunSpeed;
	BaseStat.SprintSpeed = BaseStatRow->SprintSpeed;
	BaseStat.BaseAttack = BaseStatRow->BaseAttack;
	BaseStat.BaseAttackSpeed = BaseStatRow->BaseAttackSpeed;
	BaseStat.BaseDefence = BaseStatRow->BaseDefence;
}
