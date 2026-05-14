// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SubSystems/GameInstanceSubsystem.h"
#include "Tables/PlayerEnums.h"
#include "UserDataSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FPlayerBaseStat
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	float MaxHp = 0; // MaxHp
	UPROPERTY(BlueprintReadWrite)
	float MaxStamina = 0.f; // MaxStamina
	UPROPERTY(BlueprintReadWrite)
	float StaminaRecoveryPerSecond = 0.f; // StaminaRecoveryPerSecond
	UPROPERTY(BlueprintReadWrite)
	float StaminaRecoveryDelay = 0.f; // StaminaRecoveryDelay
	UPROPERTY(BlueprintReadWrite)
	float WalkSpeed = 0.f; // WalkSpeed
	UPROPERTY(BlueprintReadWrite)
	float RunSpeed = 0.f; // RunSpeed
	UPROPERTY(BlueprintReadWrite)
	float SprintSpeed = 0.f; // SprintSpeed
	UPROPERTY(BlueprintReadWrite)
	float BaseAttack = 0.f; // BaseAttack
	UPROPERTY(BlueprintReadWrite)
	float BaseAttackSpeed = 0.f; // BaseAttackSpeed
	UPROPERTY(BlueprintReadWrite)
	float BaseDefence = 0.f; // BaseDefence
};

USTRUCT(BlueprintType)
struct FPlayerActionData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	int32 Tid = 0;
	UPROPERTY(BlueprintReadWrite)
	FName Name;
	UPROPERTY(BlueprintReadWrite)
	EPlayerActionCategory Category = EPlayerActionCategory::Movement;
	UPROPERTY(BlueprintReadWrite)
	float StaminaCost = 0.f;
	UPROPERTY(BlueprintReadWrite)
	EPlayerStaminaCostType StaminaCostType = EPlayerStaminaCostType::Instant;
	UPROPERTY(BlueprintReadWrite)
	float MinRequiredStamina = 0.f;
	UPROPERTY(BlueprintReadWrite)
	float SprintRestartStaminaPercent = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHpChanged, float, CurrentHP, float, MaxHp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, CurrentStamina, float, MaxStamina);

UCLASS()
class BAPROJECT_API UUserDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UUserDataSubsystem();
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	static UUserDataSubsystem* Get(const UObject* WorldContext);
	
	FORCEINLINE FPlayerBaseStat GetBaseStat() const { return BaseStat; }
	FORCEINLINE TMap<int32, FPlayerActionData> GetActionDataMap() const { return ActionDataMap;}
	const FPlayerActionData* FindActionData(const int32 Tid) const { return ActionDataMap.Find(Tid); }

	// UI
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "User|Event")
	FOnHpChanged OnHpChanged;
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "User|Event")
	FOnStaminaChanged OnStaminaChanged;

protected:
	UPROPERTY(BlueprintReadOnly)
	FPlayerBaseStat BaseStat;
	
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FPlayerActionData> ActionDataMap;
private:
	void SetBaseStat();
	void SetActionData();

};
