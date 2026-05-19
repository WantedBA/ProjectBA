#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "EnemyBase.generated.h"

class UStatComponent;
class UCombatComponent;

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle,
	Move,
	Chase,
	Attack,
	Hit,
	Dead
};

UENUM(BlueprintType)
enum class EEnemyGrade : uint8
{
	None,
	Elite,
	Boss
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, EEnemyState, OldState, EEnemyState, NewState);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttackAnimationFinishedDelegate, EEnemyState);
DECLARE_MULTICAST_DELEGATE(FOnEnemyDeathDelegate);

UCLASS(Abstract)
class BAPROJECT_API AEnemyBase : public ACharacterBase
{
	GENERATED_BODY()

public:
	AEnemyBase();

	virtual void InitializeFromTable(int32 InTid);

	virtual void Attack();

	UFUNCTION()
	virtual void OnDeath() override;

	virtual void OnEnemyAttackAniFinished(EEnemyState NewState);

	UFUNCTION(BlueprintPure, Category = "State")
	bool IsDead() const { return CurrentState == EEnemyState::Dead; }
	void SetSuperArmor(bool NewBool) { bIsSuperArmor = NewBool; }

	UFUNCTION(BlueprintPure, Category = "State")
	EEnemyState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "State")
	EEnemyGrade GetEnemyGrade() const { return EnemyGrade; }
	
	//한번 잡은 타겟 영구 유지 여부
	UFUNCTION(BlueprintPure, Category="AI")
	virtual bool IsPersistentAggro() const { return false; }
	
	int32 GetMonsterTid() const { return MonsterTid; }

	void SetState(EEnemyState NewState);

	UAnimMontage* GetEnemyAttackMontage() const { return AttackMontage; }

	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif
	virtual void UpdateMoveSpeed(EEnemyState NewState);
	virtual void UpdateBlackBoardState();
	virtual void ApplyKnockback(AActor* DamageCauser, float Force);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void HandlePerfectGuarded(FVector ImpactLocation);

protected:
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;

	virtual void OnDamaged(float FinalDamage, AActor* DamageCauser) override;

	// 시각 연출 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Visuals", meta = (DisplayName = "OnHitVisuals"))
	void K2_OnHitVisuals(FVector HitLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Visuals", meta = (DisplayName = "OnDeadVisuals"))
	void K2_OnDeadVisuals();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat", meta = (DisplayName = "OnPerfectGuarded"))
	void K2_OnPerfectGuarded(FVector ImpactLocation);

public:
	FOnAttackAnimationFinishedDelegate OnAttackAnimationFinished;
	FOnEnemyDeathDelegate OnDeathEvent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	int32 MonsterTid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	float DetectRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugRanges = true;

	UPROPERTY(BlueprintAssignable, Category = "State")
	FOnStateChanged OnStateChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	EEnemyState CurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	EEnemyGrade EnemyGrade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bIsSuperArmor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Effects")
	TObjectPtr<class UNiagaraSystem> PerfectDefenseVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Effects")
	TObjectPtr<USoundBase> PerfectDefenseSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Effects")
	TSubclassOf<class UCameraShakeBase> PerfectDefenseCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> PerfectGuardedMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	UAnimMontage* HitMontage;

	float MaxMoveSpeed;
};
