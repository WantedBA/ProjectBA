#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Player/BAPlayerCharacterTypes.h"
#include "Tables/ActionEnums.h"
#include "BAPlayerCharacter.generated.h"

class UPlayerWeaponVFX;
class UNiagaraComponent;
class UCombatComponent;
class UPlayerSkillComponent;
class UAnimMontage;
class UCameraComponent;
class UCharacterMovementComponent;
class UActionComponent;
class UActionAnimationComponent;
class UInteractorComponent;
class AMapLadder;
class USpringArmComponent;
class UStatComponent;
class UStaticMeshComponent;
class UTargetComponent;

/**
 * 플레이어 캐릭터 본체.
 *
 * CharacterBase의 전투 진입점을 유지하면서 플레이어 전용 이동 상태,
 * 스태미너 기반 질주, 사다리 상호작용, 카메라 및 공용 컴포넌트를 묶어 관리한다.
 * 애니메이션 블루프린트는 이 클래스의 BlueprintPure getter로 이동/전투 상태를 조회한다.
 */
 
// 현재 재생 중인 애니메이션(컴포넌트로 들어가기 전에 if문으로 분기) - 추후 제거하고 컴포넌트와 통합 TODO
UENUM(BlueprintType)
enum class EBAPlayerState : uint8
{
	None,
	Attacking,
	Guarding,
	Moving, 
	DodgeRolling,
	HitReacting,
	KnockedDown,
	Dead
};

UCLASS()
class BAPROJECT_API ABAPlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	ABAPlayerCharacter();

	// 생명주기
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 공격 관련
	virtual void TryAttack(EActionCommand InActionCommand); // CharacterBase 공격 진입점. 
	
	// 차징 공격 판정
	void ChargeAttackStart();
	void ChargeLoopStart(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);
	void ChargeAttackCompleted();
	void StopChargeEffect();

	bool TryStartGuard();
	void StopGuard();
	void CancelGuardForSprintInput();
	void ConsumePerfectGuardStaminaCost();
	void SetGuardWindowActive(bool bActive);
	void SetPerfectGuardWindowActive(bool bActive);
	virtual bool IsGuardingAgainstDamage(const FVector& DamageDirection) const override;
	virtual bool IsPerfectGuardWindowActive() const override;

	void OnAttackMontageEnded(UAnimMontage* AnimMontage, bool bArg);
	void StartAttack(UAnimMontage* InAnimMontage);
	
	virtual class UStaticMeshComponent* GetWeaponMesh() const override { return WeaponMeshComponent; }

	// NextComboTransitionTid와 NextAttackMontage를 설정하는 함수
	void SetNextCombo(EActionCommand InActionCommand);
	void OnNextComboCheck();
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetBAPlayerState(EBAPlayerState NewState);
	
	// 초기화
	UFUNCTION(BlueprintCallable, Category = Initialization)
	virtual void InitializeFromTable();

	// 이동 입력 및 제어
	void SetMovementState(EMovementState NewState);
	void SetMoveInputVector(const FVector2D& NewMoveInput);
	void SetHasMoveInput(bool bNewHasMoveInput);

	UFUNCTION(BlueprintCallable, Category = "Animation|Movement")
	void SetLocomotionMode(EPlayerLocomotionMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void SetCombatMode(EPlayerCombatMode NewMode);

	// 이동 애니메이션 상태 조회
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EMovementState GetMovementState() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EMovementState GetDesiredMovementState() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EPlayerLocomotionMode GetLocomotionMode() const;

	UFUNCTION(BlueprintPure, Category = "LockOn")
	bool IsLockOnTargetLocked() const;

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ToggleLockOnTargeting();

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void SwitchLockOnTargetInput(const FVector2D& SwitchInput);

	// Start, Loop, Stop, Turn 등 현재 재생해야 하는 이동 페이즈를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EPlayerMovementPhase GetMovementPhase() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Combat")
	EPlayerCombatMode GetCombatMode() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Combat")
	bool CanMoveWhileGuarding() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Combat")
	bool ShouldUseUpperBodyGuardPose() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	bool HasMoveInput() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	FVector2D GetMoveInputVector() const;

	EActionDirection GetActionDirectionFromMoveInput(const FVector2D& MoveInput) const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	FVector GetMoveInputWorldDirection() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	FVector GetMoveInputLocalDirection() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetMoveInputDirectionAngle() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetVelocityDirectionAngle() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetGroundSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetMovementPhaseDirectionAngle() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	bool IsMovementPhase(EPlayerMovementPhase Phase) const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	bool IsMovementPhaseUsingRootMotion() const;

	UFUNCTION(BlueprintCallable, Category = "Animation|Movement")
	void CompleteMovementPhaseAnimation(EPlayerMovementPhase CompletedPhase);

	UFUNCTION(BlueprintPure, Category = "Animation|Sprint")
	bool IsSprintLockedAfterExhausted() const;

	// 주요 컴포넌트 접근
	UFUNCTION(BlueprintPure, Category = "Interaction")
	UInteractorComponent* GetInteractorComponent() const { return InteractorComponent; }

	UFUNCTION(BlueprintPure, Category = "Action")
	UActionComponent* GetActionComponent() const { return ActionComponent; }

	UFUNCTION(BlueprintPure, Category = "Action")
	UActionAnimationComponent* GetActionAnimationComponent() const { return ActionAnimationComponent; }

	// 피격 반응 상태 조회
	UFUNCTION(BlueprintPure, Category = "Combat|DamageReaction")
	bool IsDamageReacting() const;

	UFUNCTION(BlueprintPure, Category = "Combat|DamageReaction")
	EPlayerDamageReactionState GetDamageReactionState() const;

	UFUNCTION(BlueprintPure, Category = "Action")
	bool CanAcceptActionInput() const;

	// 사다리 상호작용
	UFUNCTION(BlueprintCallable, Category = "Interaction|Ladder")
	void EnterLadder(AMapLadder* Ladder, const FVector& EntryLocation, const FRotator& FaceRotation);

	UFUNCTION(BlueprintCallable, Category = "Interaction|Ladder")
	void ExitLadder(const FVector& ExitLocation);

	UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
	bool IsOnLadder() const;

	UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
	AMapLadder* GetCurrentLadder() const;

	UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
	float GetLadderClimbVelocity() const;

	// 컷신 제어
	UFUNCTION(BlueprintCallable, Category = "Control|Cutscene")
	void LockMovementForCutscene();

	UFUNCTION(BlueprintCallable, Category = "Control|Cutscene")
	void UnlockMovementForCutscene();

	/** 마지막 체크포인트에서 부활 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Respawn();

protected:
	// CharacterBase 훅
	virtual void PostInitializeComponents() override;
	virtual void OnDamaged(
		float FinalDamage,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION()
	virtual void OnDeath() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	EBAPlayerState BAPlayerState = EBAPlayerState::None;
	
	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractorComponent> InteractorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UActionComponent> ActionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UActionAnimationComponent> ActionAnimationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerSkillComponent> PlayerSkillComponent;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	TObjectPtr<UPlayerWeaponVFX> PlayerWeaponVFX;

	// 스탯 및 액션 콜백
	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);

	UFUNCTION()
	void OnStaminaChanged(float CurrentStamina, float MaxStamina);

	void BindActionCallbacks();
	void BindGuardActionCallbacks();
	void BindDodgeActionCallbacks();
	void BindLockOnTargetCallbacks();
	void ConfigureLockOnCameraDefaults();

	UFUNCTION()
	void HandleLockOnTargetLocked(UTargetComponent* Target, FName Socket);

	UFUNCTION()
	void HandleLockOnTargetUnlocked(UTargetComponent* UnlockedTarget, FName Socket);

	UFUNCTION()
	void HandleActionStarted(int32 ActionTid, EActionType ActionType);

	UFUNCTION()
	void HandleGuardInterruptingActionStarted(int32 ActionTid, EActionType ActionType);

	UFUNCTION()
	void HandleDodgeActionStarted(int32 ActionTid, EActionType ActionType);

	UFUNCTION()
	void HandleGuardActionMontageEnded(int32 ActionTid, EActionType ActionType, UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleDodgeActionMontageEnded(int32 ActionTid, EActionType ActionType, UAnimMontage* Montage, bool bInterrupted);

	// 피격 반응을 C++ 기본 처리 이후 블루프린트 연출로 확장한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|DamageReaction", meta = (DisplayName = "OnDamageReaction"))
	void K2_OnDamageReaction(
		EBADamageReactionType DamageReactionType,
		EActionDirection HitDirection,
		bool bGuarding,
		bool bGuardBreak);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Guard", meta = (DisplayName = "OnPerfectGuardSucceeded"))
	void K2_OnPerfectGuardSucceeded(const FHitResult& HitResult, AActor* DamageCauser);
	
	// 공격 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 FirstLComboTransitionTid = 71001;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 FirstRComboTransitionTid = 72001;
	
	const float WeaponRadius = 20.f; // 충돌 판정 시 검 두께
	int32 NowComboTransitionTid = 0; // 다음 콤보 결정할 때 사용
	int32 NextComboTransitionTid = 0; // 결정된 다음 콤보 저장
	
	UPROPERTY()
	TObjectPtr<UAnimMontage> NextAttackMontage = nullptr;
	EActionType NextAttackActionType = EActionType::None;
	
	// 최대 차징 시간
	UPROPERTY(EditAnywhere, Category="Combat")
	float MaxChargeTime = 1.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FTimerHandle ChargeAttackTimerHandle;
	
private:
	// 이동 런타임
	void TickMovementRuntime(float DeltaTime);
	void UpdatePhaseFromInputAndGait(float DeltaTime);
	void BeginMovementPhase(EPlayerMovementPhase NewPhase);
	void FinishCurrentMovementPhase();
	void SetActiveGaitAndSpeed(EMovementState NewGait);
	void UpdateMaxWalkSpeed(float DeltaTime);
	EMovementState GetMovementAllowedGait(EMovementState RequestedGait) const;
	const FBAPlayerMovementPhaseSettings& GetPhaseSettings(EMovementState Gait) const;
	float GetSpeedForGait(EMovementState Gait) const;
	bool IsPhaseEnabledForGait(EPlayerMovementPhase Phase, EMovementState Gait) const;
	bool DoesGaitPhaseUseRootMotion(EPlayerMovementPhase Phase, EMovementState Gait) const;
	bool ShouldEnterTurnPhase() const;
	float CalculateInputYawDeltaFromActor() const;

	// 이동 입력 버퍼와 회전
	void ApplyBufferedMoveInput();
	bool IsActionMovementLocked() const;
	void SyncFreeStrafeFacingMode();
	void EnterLockOnStrafeMode();
	bool ShouldDelayLockOnStrafeMode() const;
	void ApplyPendingLockOnStrafeMode();
	void RestoreLocomotionModeAfterLockOn();
	void UpdateInterpolatedFacingRotation(float DeltaTime);
	bool ShouldUseInterpolatedFacingRotation() const;
	bool ShouldBlendMovementFacingRotation() const;
	void UseMovementDirectionFacing(UCharacterMovementComponent& MovementComponent);
	void UseControllerYawFacing(UCharacterMovementComponent& MovementComponent);
	void UpdateInterpolatedMoveInputDirection(float DeltaTime);
	void SnapInterpolatedMoveInputTo(const FVector2D& MoveInput);
	FVector2D GetInterpolatedMoveInputVector() const;
	void FaceMoveInputDirection();
	EActionDirection ResolveBufferedActionDirection(int32 ActionTid, EActionDirection BufferedDirection) const;
	EActionDirection ResolveActionAnimationDirection(int32 ActionTid, EActionDirection ActionDirection) const;
	EActionDirection ResolveActionOrientationDirection(int32 ActionTid, EActionDirection ActionDirection) const;
	FVector2D ConvertWorldDirectionToMoveInput(const FVector& WorldDirection) const;
	FVector ConvertMoveInputToWorldDirection(const FVector2D& MoveInput) const;

	// 질주 스태미너
	bool IsSprintAllowedByStamina() const;
	void DrainSprintStaminaDuringLoop(float DeltaTime);
	float CalculateSprintStaminaDrain(float DeltaTime) const;
	void PauseSprintStaminaRecovery();
	void ResumeSprintStaminaRecovery(bool bApplyDelay);
	void LockSprintUntilRecovered();
	void UnlockSprintAfterRecovery();

	// 가드
	void CancelGuardForActionInterrupt();
	void ConfigureGuardMontageSections();
	float GetGuardAbsorptionMultiplier() const;
	float GetPerfectGuardStaminaCostMultiplier() const;
	bool ConsumeGuardStaminaForDamage();
	void KeepGuardActiveAfterGuardSuccess();
	bool ShouldResumeGuardAfterGuardReaction() const;
	bool ResumeGuardAfterGuardReaction();
	void ShowGuardJudgementDebugMessage(const FString& Message, const FColor& Color) const;

	// 사다리 런타임
	void TickLadderClimb(float DeltaTime);
	float CalculateLadderClimbSpeed(float VerticalInput) const;
	bool IsLadderSprintRequested() const;
	void DrainLadderSprintStamina(float DeltaTime);
	void ResetMovementRuntimeForLadder();

	// 피격 반응
	bool ShouldPlayGuardBreakReaction() const;
	void HandlePerfectGuardSucceeded(const FHitResult& HitResult, AActor* DamageCauser);
	void CancelCurrentActionForDamageReaction();
	void PlayDamageReactionAnimation(
		EBADamageReactionType DamageReactionType,
		EActionDirection HitDirection,
		bool bGuarding,
		bool bGuardBreak);
	EPlayerDamageReactionState ResolveDamageReactionState(
		EBADamageReactionType DamageReactionType,
		bool bGuarding,
		bool bGuardBreak) const;
	UAnimMontage* SelectDamageReactionMontage(
		EBADamageReactionType DamageReactionType,
		EActionDirection HitDirection,
		bool bGuarding,
		bool bGuardBreak) const;
	void HandleGuardHitReactionMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 PlaybackId);
	void ApplyDamageReactionKnockback(
		EBADamageReactionType DamageReactionType,
		const FVector& DamageDirection,
		EActionDirection HitDirection,
		bool bGuarding,
		bool bGuardBreak);
	void FinishDamageReaction(int32 PlaybackId);

	// 이동 설정
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ShowOnlyInnerProperties))
	FBAPlayerMovementSpeedSettings SpeedSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion", meta = (ShowOnlyInnerProperties))
	FBAPlayerLocomotionSettings LocomotionSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Gait", meta = (ShowOnlyInnerProperties))
	FBAPlayerMovementGaitSettings GaitSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Sprint", meta = (ShowOnlyInnerProperties))
	FBAPlayerSprintCostSettings SprintCostSettings;

	UPROPERTY(EditAnywhere, Category = "Interaction|Ladder", meta = (ShowOnlyInnerProperties))
	FBAPlayerLadderSettings LadderSettings;

	UPROPERTY(EditAnywhere, Category = "LockOn|Movement")
	bool bForceStrafeWhileLockedOn = true;

	UPROPERTY(EditAnywhere, Category = "LockOn|Camera")
	bool bAutoCalibrateLockOnControllerPitch = true;

	UPROPERTY(EditAnywhere, Category = "LockOn|Camera", meta = (Units = "deg"))
	float LockOnAdditionalControllerPitchOffset = 0.f;

	// 피격 반응 설정
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float GuardDamageBlockAngle = 120.f;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction")
	float DamageReactionFallbackDuration = 0.6f;

	// Deprecated: 피격/가드 히트 Launch 넉백은 항상 적용한다. 직렬화된 BP 설정 호환을 위해 필드는 유지한다.
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	bool bUseLaunchKnockbackForDamageReaction = false;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float HitReactKnockbackStrength = 250.f;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float LargeHitReactKnockbackStrength = 500.f;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float KnockDownKnockbackStrength = 650.f;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float GuardHitKnockbackStrength = 360.f;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float GuardBreakKnockbackStrength = 650.f;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float DamageReactionKnockbackZ = 20.f;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> HitReactMontages;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> LargeHitReactMontages;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> KnockDownReactMontages;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> GuardHitReactMontages;

	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> GuardBreakReactMontages;

	UPROPERTY(EditAnywhere, Category = "Combat|Guard|Montage")
	FName GuardLoopSection = TEXT("Loop");

	// 런타임 상태
	FBAPlayerMovementRuntimeState MovementRuntime;
	FBAPlayerSprintRuntimeState SprintRuntime;
	FBAPlayerLadderRuntimeState LadderRuntime;
	EPlayerLocomotionMode LocomotionModeBeforeLockOn = EPlayerLocomotionMode::Free;
	EPlayerDamageReactionState DamageReactionState = EPlayerDamageReactionState::None;
	FTimerHandle DamageReactionTimerHandle;
	int32 ActiveDamageReactionPlaybackId = 0;
	int32 NextDamageReactionPlaybackId = 1;
	bool bGuardInputHeld = false;
	bool bPerfectGuardWindowActive = false;
	bool bLockOnForcedStrafeActive = false;
	bool bPendingLockOnStrafeAfterDodge = false;
	UPROPERTY(VisibleAnywhere) bool bIsBeforeCharge = false;
	UPROPERTY(VisibleAnywhere) bool bIsCharging = false;
	UPROPERTY(VisibleAnywhere) bool bIsChargeInputCompleted = false;
	UPROPERTY(Transient) UAnimMontage* PausedMontage = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDamageReactionMontage;
};
