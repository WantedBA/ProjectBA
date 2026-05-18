#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, CurrentStamina, float, MaxStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDead);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatComponent();

public:
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnHPChanged OnHPChanged;
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnStaminaChanged OnStaminaChanged;
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnDead OnDead;

	UFUNCTION(BlueprintCallable, Category = "Stat")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Stat")
	void InitializeStats(float InMaxHP, float InAttack, float InDefence);
	void InitializeStats
	(
		const float InMaxHP, 
		const float InMaxStamina, 
		const float InStaminaRecoveryPerSecond,
		const float InStaminaRecoveryDelay,
		const float InWalkSpeed, 
		const float InRunSpeed, 
		const float InSprintSpeed, 
		const float InAttack, 
		const float InAttackSpeed, 
		const float InDefence
	);

	UFUNCTION(BlueprintPure, Category = "Stat")
	bool IsDead() const { return CurrentHP <= 0.f; }

	FORCEINLINE float GetAttack() const { return Attack; }
	FORCEINLINE void SetAttack(const float NewAttack) { Attack = NewAttack; }

	FORCEINLINE float GetDefence() const { return Defence; }
	FORCEINLINE void SetDefence(const float NewDefence) { Defence = NewDefence; }

	FORCEINLINE float GetMaxHP() const { return MaxHP; }
	FORCEINLINE void SetMaxHP(const float NewMaxHP) { MaxHP = NewMaxHP; }
	FORCEINLINE float GetCurrentHP() const { return CurrentHP; }
	FORCEINLINE void SetCurrentHP(const float NewCurrentHP) { CurrentHP = NewCurrentHP; }

	FORCEINLINE float GetMaxStamina() const { return MaxStamina; }
	FORCEINLINE void SetMaxStamina(const float NewMaxStamina) { MaxStamina = NewMaxStamina; }
	FORCEINLINE float GetCurrentStamina() const { return CurrentStamina; }
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void ConsumeStamina(float ConsumeAmount);
	void SetCurrentStamina(const float NewCurrentStamina);

	FORCEINLINE float GetWalkSpeed() const { return WalkSpeed; }
	FORCEINLINE float GetRunSpeed() const { return RunSpeed; }
	FORCEINLINE float GetSprintSpeed() const { return SprintSpeed; }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Health")
	float MaxHP;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Health")
	float CurrentHP;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Stamina")
	float MaxStamina;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Stamina")
	float CurrentStamina;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Stamina")
	float StaminaRecoveryPerSecond;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Stamina")
	float StaminaRecoveryDelay;

	float StaminaRecoveryDelayRemaining = 0.f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Movement")
	float WalkSpeed;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Movement")
	float RunSpeed;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Movement")
	float SprintSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Attack")
	float Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Attack")
	float AttackSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat|Defence")
	float Defence;
};
