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

	UWorld* World = WorldContext->GetWorld();
	if (!World) return nullptr;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return nullptr;

	return GI->GetSubsystem<UUserDataSubsystem>();
}

void UUserDataSubsystem::SetBaseStat()
{
	//BaseStat.MaxHp = // 테이블에서 온 데이터 추가
	
	// 아래는 TableManager 호출 예시
	UBATableManager* TableManager = GetGameInstance()->GetSubsystem<UBATableManager>();
	if (!TableManager) return;
	
	//BaseStat은 TableManager에서 가져온 값으로 설정
	
	CurMaxHp = BaseStat.MaxHp;
	CurMaxStamina  = BaseStat.MaxStamina;
}
