#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDead);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatComponent();

	UFUNCTION(BlueprintCallable, Category = "Stat")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Stat")
	void InitializeStats(float InMaxHP, float InAttack, float InDefence);
	void InitializeStats
	(
		const float InMaxHP, 
		const float InMaxStamina, 
		const float InMoveSpeed, 
		const float InAttack, 
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
	FORCEINLINE void SetCurrentStamina(const float NewCurrentStamina) { CurrentStamina = NewCurrentStamina; }

	FORCEINLINE float GetMoveSpeed() const { return MoveSpeed; }
	FORCEINLINE void SetMoveSpeed(const float NewMoveSpeed) { MoveSpeed = NewMoveSpeed; }
public:
	UPROPERTY(BlueprintAssignable, Category = "Stat|Event")
	FOnHPChanged OnHPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stat|Event")
	FOnDead OnDead;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float MaxHP;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float CurrentHP;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float MaxStamina;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float CurrentStamina;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float MoveSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float Attack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float Defence;
};
