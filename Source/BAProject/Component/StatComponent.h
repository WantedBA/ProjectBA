#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, CurrentStamina, float, MaxStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDead);

/**
 * 체력, 스태미너, 이동 속도, 공격/방어 스탯을 보관하고 변경 이벤트를 발행하는 컴포넌트.
 *
 * 스태미너는 소비 후 지연 시간을 거쳐 초당 비율로 회복되며, 액션/질주 같은 시스템은
 * Source 이름으로 회복을 일시정지하거나 재개할 수 있다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatComponent();

public:
	// HP 값이 변경될 때 현재값과 최대값을 함께 알린다.
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnHPChanged OnHPChanged;

	// 스태미너 값이 변경될 때 현재값과 최대값을 함께 알린다.
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnStaminaChanged OnStaminaChanged;

	// HP가 0 이하가 되는 순간 알린다.
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnDead OnDead;

	// 방어력을 차감한 최종 피해를 HP에 적용한다.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void ApplyDamage(float DamageAmount);

	// HP, 공격력, 방어력만 사용하는 간단 초기화 경로.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void InitializeStats(float InMaxHP, float InAttack, float InDefence);

	// 플레이어 기본 스탯 전체를 초기화하고 HP/스태미너를 최대치로 채운다.
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

	// 모든 스탯을 최대치로 복구하고 이벤트를 발행한다.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void RestoreAll();

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

	// 스태미너를 지정량 소비하고 회복 Tick 상태를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void ConsumeStamina(float ConsumeAmount);

	// Source를 회복 일시정지 목록에 추가한다. 여러 Source가 모두 해제되어야 회복된다.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void PauseStaminaRecovery(FName Source);

	// Source를 회복 일시정지 목록에서 제거하고 필요하면 회복 지연 시간을 다시 적용한다.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void ResumeStaminaRecovery(FName Source, bool bApplyDelay = true);

	// 하나 이상의 Source가 스태미너 회복을 막고 있는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Stat")
	bool IsStaminaRecoveryPaused() const { return !StaminaRecoveryPauseSources.IsEmpty(); }

	// 스태미너 현재값을 범위 안으로 보정하고 변경 이벤트를 발행한다.
	void SetCurrentStamina(const float NewCurrentStamina);

	FORCEINLINE float GetWalkSpeed() const { return WalkSpeed; }
	FORCEINLINE float GetRunSpeed() const { return RunSpeed; }
	FORCEINLINE float GetSprintSpeed() const { return SprintSpeed; }

protected:
	// 스태미너 회복이 필요한 동안만 활성화된다.
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
	TSet<FName> StaminaRecoveryPauseSources;
	
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

private:
	void RefreshStaminaRecoveryTick();
};
