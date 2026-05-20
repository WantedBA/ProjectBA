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
	Alert,
	Attack,
	Hit,
	Stagger,
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDissolveStarted);

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

	int32 GetMonsterTid() const { return MonsterTid; }

	void SetState(EEnemyState NewState);

	UAnimMontage* GetEnemyAttackMontage() const { return AttackMontage; }

	float GetMaxChaseDistance() { return MaxChaseDistance; }

	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif
	virtual void UpdateMoveSpeed(EEnemyState NewState);
	virtual void UpdateBlackBoardState();
	virtual void ApplyKnockback(AActor* DamageCauser, float Force);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void HandlePerfectGuarded(FVector ImpactLocation);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Effects")
	void OnStartDissolve();
	bool CanAttack() const; // 공격 횟수 및 쿨다운 관리
	void ResetAttackCount() { CurrentAttackCount = 0; }

	void ResetStateToIdle();// 상태 복구 관리

protected:
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;

	virtual void OnDamaged(float FinalDamage, AActor* DamageCauser) override;

	// 시각 연출 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Visuals", meta = (DisplayName = "OnHitVisuals"))
	void K2_OnHitVisuals(FVector HitLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Visuals", meta = (DisplayName = "OnDeadVisuals"))
	void K2_OnDeadVisuals();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Visuals", meta = (DisplayName = "OnPerfectGuarded"))
	void K2_OnPerfectGuarded(FVector ImpactLocation);

public:
	FOnAttackAnimationFinishedDelegate OnAttackAnimationFinished;
	FOnEnemyDeathDelegate OnDeathEvent;

protected:
	bool bInitAI = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	float MaxChaseDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 MaxAttackCount = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 CurrentAttackCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AlertDuration = 3.0f;

	FTimerHandle StateTimerHandle;

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
	TObjectPtr<UAnimMontage> HitMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> DeadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TArray<UMaterialInterface*> DissolveMaterialsInput;

	UPROPERTY(BlueprintReadOnly, Category = "Effects")
	TArray<UMaterialInstanceDynamic*> DynamicDissolveMaterials;

	UPROPERTY(BlueprintAssignable, Category = "Effects")
	FOnDissolveStarted OnDissolveStarted;

	float MaxMoveSpeed;
};
