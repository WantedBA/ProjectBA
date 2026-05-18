// Copyright TeamBA. All Rights Reserved.

#include "UserDataSubsystem.h"
#include "Tables/BATableManager.h"

UUserDataSubsystem::UUserDataSubsystem()
{
}

void UUserDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
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

void UUserDataSubsystem::NotifyPlayerStatChanged(float CurrentHP, float MaxHP, float CurrentStamina, float MaxStamina)
{
	// 캐릭터로부터 받은 데이터를 UI용 채널로 전달
	if (OnPlayerHpChanged.IsBound())
	{
		OnPlayerHpChanged.Broadcast(CurrentHP, MaxHP);
	}

	if (OnPlayerStaminaChanged.IsBound())
	{
		OnPlayerStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	}
}

void UUserDataSubsystem::SetBaseStat()
{
	const UBATableManager* TableManager = UBATableManager::Get(this);
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

	// 사다리 관련 스탯도 함께 초기화
	BaseStat.LadderClimbSpeedSlow = BaseStatRow->LadderClimbSpeedSlow;
	BaseStat.LadderClimbSpeedFast = BaseStatRow->LadderClimbSpeedFast;
	BaseStat.LadderSlideDownSpeed = BaseStatRow->LadderSlideDownSpeed;
	BaseStat.LadderExitClearance = BaseStatRow->LadderExitClearance;

	UE_LOG(LogTemp, Log, TEXT("[UserDataSubsystem] BaseStat loaded."));
}
