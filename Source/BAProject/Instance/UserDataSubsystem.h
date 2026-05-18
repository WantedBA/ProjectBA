// Copyright TeamBA. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SubSystems/GameInstanceSubsystem.h"
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
	UPROPERTY(BlueprintReadWrite)
	float LadderClimbSpeedSlow = 0.f; // LadderClimbSpeedSlow
	UPROPERTY(BlueprintReadWrite)
	float LadderClimbSpeedFast = 0.f; // LadderClimbSpeedFast
	UPROPERTY(BlueprintReadWrite)
	float LadderSlideDownSpeed = 0.f; // LadderSlideDownSpeed
	UPROPERTY(BlueprintReadWrite)
	float LadderExitClearance = 0.f; // LadderExitClearance
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerHpChanged, float, CurrentHP, float, MaxHp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerStaminaChanged, float, CurrentStamina, float, MaxStamina);

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

	// UI
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "User|Event")
	FOnPlayerHpChanged OnPlayerHpChanged;
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "User|Event")
	FOnPlayerStaminaChanged OnPlayerStaminaChanged;

	UFUNCTION(BlueprintCallable, Category = "User|Update")
	void NotifyPlayerStatChanged(float CurrentHP, float MaxHP, float CurrentStamina, float MaxStamina);

protected:
	UPROPERTY(BlueprintReadOnly)
	FPlayerBaseStat BaseStat;
private:
	void SetBaseStat();

};
