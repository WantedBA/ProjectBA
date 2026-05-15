// Copyright TeamBA. All Rights Reserved.

#include "UserDataSubsystem.h"
#include "Tables/BATableManager.h"

UUserDataSubsystem::UUserDataSubsystem()
{
}

void UUserDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	//SubSystem간 순서보장을 위한 의존성 설정 
	Collection.InitializeDependency<UBATableManager>(); 
	
	Super::Initialize(Collection);
	
	SetBaseStat();
	SetActionData();
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

	BaseStat.MaxHp = BaseStatRow->MaxHp;
	BaseStat.MaxStamina = BaseStatRow->MaxStamina;
	BaseStat.StaminaRecoveryPerSecond = BaseStatRow->StaminaRecoveryPerSecond;
	BaseStat.StaminaRecoveryDelay = BaseStatRow->StaminaRecoveryDelay;
	BaseStat.WalkSpeed = BaseStatRow->WalkSpeed;
	BaseStat.RunSpeed = BaseStatRow->RunSpeed;
	BaseStat.SprintSpeed = BaseStatRow->SprintSpeed;
	BaseStat.BaseAttack = BaseStatRow->BaseAttack;
	BaseStat.BaseAttackSpeed = BaseStatRow->BaseAttackSpeed;
	BaseStat.BaseDefence = BaseStatRow->BaseDefence;
	
	UE_LOG(LogTemp, Log, TEXT("[UserDataSubsystem] BaseStat loaded."));
}

void UUserDataSubsystem::SetActionData()
{
	const UBATableManager* TableManager = GetGameInstance()->GetSubsystem<UBATableManager>();
	if (!TableManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[UserDataSubsystem] TableManager subsystem is null."));
		return;
	}

	const TMap<int32, FPlayerActionDataRow*>& RowMap = TableManager->GetPlayerActionDataTable();
	if (RowMap.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UserDataSubsystem] Player ActionData table is empty."));
		return;
	}

	ActionDataMap.Reset();
	for (const TPair<int32, FPlayerActionDataRow*>& Pair : RowMap)
	{
		const FPlayerActionDataRow* Row = Pair.Value;
		if (!Row) continue;

		FPlayerActionData Data;
		Data.Tid                = Row->Tid;
		Data.Name               = Row->Name;
		Data.Category           = Row->Category;
		Data.StaminaCost        = Row->StaminaCost;
		Data.StaminaCostType    = Row->StaminaCostType;
		Data.MinRequiredStamina = Row->MinRequiredStamina;
		Data.SprintRestartStaminaPercent = Row->SprintRestartStaminaPercent;
		ActionDataMap.Add(Row->Tid, Data);
	}

	UE_LOG(LogTemp, Log, TEXT("[UserDataSubsystem] ActionData loaded. Count=%d"), ActionDataMap.Num());
}
