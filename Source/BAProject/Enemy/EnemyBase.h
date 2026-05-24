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
	Dead,
	Tactical
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

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	bool IsDead() const { return CurrentState == EEnemyState::Dead; }
	void SetSuperArmor(bool NewBool) { bIsSuperArmor = NewBool; }

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	EEnemyState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Enemy|State")
	EEnemyGrade GetEnemyGrade() const { return EnemyGrade; }
	
	//한번 잡은 타겟 영구 유지 여부
	UFUNCTION(BlueprintPure, Category="AI")
	virtual bool IsPersistentAggro() const { return false; }
	
	int32 GetMonsterTid() const { return MonsterTid; }

	void SetState(EEnemyState NewState);

	UAnimMontage* GetEnemyAttackMontage() const { return AttackMontage; }
	UAnimMontage* GetEnemyHitMontage() const { return HitMontage; }
	UAnimMontage* GetEnemyStaggerMontage() const { return PerfectGuardedMontage; }

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

	virtual void OnDamaged(float FinalDamage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	void HandleAttackPerfectGuarded(AActor* GuardingActor, const FHitResult& HitResult);
	void ScheduleLockOnTargetingReleaseOnDeath();
	void ReleaseLockOnTargetingOnDeath();

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
	float MaxMoveSpeed;
	FTimerHandle StateTimerHandle;
	FTimerHandle LockOnReleaseDelayTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Debug")
	bool bShowDebugState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Debug")
	bool bShowDebugRanges = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|LockOn")
	bool bDisableLockOnCaptureOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|LockOn", meta = (ClampMin = "0.0", Units = "s"))
	float LockOnReleaseDelayOnDeath = 0.75f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components")
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components")
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Data")
	int32 MonsterTid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Data")
	EEnemyGrade EnemyGrade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Data")
	float DetectRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Data")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Data")
	float MaxChaseDistance;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|State")
	FOnStateChanged OnStateChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	int32 MaxAttackCount = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	int32 CurrentAttackCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AlertDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	bool bIsSuperArmor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Effects")
	TObjectPtr<class UNiagaraSystem> HitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Effects")
	float HitVFXScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Effects")
	TObjectPtr<class UNiagaraSystem> HeavyHitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Effects")
	float HeavyHitVFXScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Effects")
	TObjectPtr<class UNiagaraSystem> PerfectDefenseVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Effects")
	TObjectPtr<USoundBase> PerfectDefenseSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat|Effects")
	TSubclassOf<class UCameraShakeBase> PerfectDefenseCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UAnimMontage> PerfectGuardedMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UAnimMontage> HitMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	TObjectPtr<UAnimMontage> DeadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Effects")
	TArray<UMaterialInterface*> DissolveMaterialsInput;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Effects")
	TArray<UMaterialInstanceDynamic*> DynamicDissolveMaterials;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Effects")
	FOnDissolveStarted OnDissolveStarted;
};
