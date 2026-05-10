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

	UFUNCTION(BlueprintPure, Category = "Stat")
	bool IsDead() const { return CurrentHP <= 0.f; }

	float GetAttack() const { return Attack; }
	void SetAttack(float NewAttack) { Attack = NewAttack; }

	float GetDefence() const { return Defence; }
	void SetDefence(float NewDefence) { Defence = NewDefence; }

	float GetCurrentHP() const { return CurrentHP; }
	void SetCurrentHP(float NewCurrentHP) { CurrentHP = NewCurrentHP; }

	float GetMaxHP() const { return MaxHP; }
	void SetMaxHP(float NewMaxHP) { MaxHP = NewMaxHP; }

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
	float Attack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stat")
	float Defence;
};
