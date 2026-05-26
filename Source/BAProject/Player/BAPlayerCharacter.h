#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimEnums.h"
#include "Character/CharacterBase.h"
#include "Player/BAPlayerCharacterTypes.h"
#include "Tables/ActionEnums.h"
#include "BAPlayerCharacter.generated.h"

class UPlayerWeaponVFX;
class UNiagaraComponent;
class UCombatComponent;
class UPlayerSkillComponent;
class UAnimMontage;
class UCameraShakeBase;
class UCameraComponent;
class UCharacterMovementComponent;
class UActionComponent;
class UActionAnimationComponent;
class UCameraOcclusionFadeComponent;
class UInteractorComponent;
class UAnimInstance;
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
	Respawning,
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

	void OnAttackMontageEnded(UAnimMontage* AnimMontage, bool bInterrupted, int32 PlaybackId);
	void StartAttack(UAnimMontage* InAnimMontage);
	
	virtual class UStaticMeshComponent* GetWeaponMesh() const override { return WeaponMeshComponent; }

	// NextComboTransitionTid와 NextAttackMontage를 설정하는 함수
	void SetNextCombo(EActionCommand InActionCommand);
	void OnNextComboCheck();

	// 스킬 컴포넌트에서 콤보 구성을 오버라이드 하기 위한 함수
	void OverrideComboTransition(int32 InNowComboTid, EActionCommand ActionCommand, int32 NewNextComboTid);
	void ResetComboTransitionOverrides();
	
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

	UFUNCTION(BlueprintCallable, Category = "Combat|Recovery")
	void OpenRecoveryEscapeWindow(const FBAPlayerRecoveryEscapeWindowSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Combat|Recovery")
	void CloseRecoveryEscapeWindow(const FBAPlayerRecoveryEscapeWindowSettings& Settings);

	UFUNCTION(BlueprintPure, Category = "Combat|Recovery")
	bool IsRecoveryEscapeWindowOpen() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Recovery")
	bool IsRecoveryEscapeRequiredForCurrentState() const;

	bool TryStartRecoveryEscapeAction(EActionCommand Command, EActionDirection Direction = EActionDirection::Any);
	bool TryStartRecoveryEscapeMove(const FVector2D& MoveInput);

	UFUNCTION(BlueprintPure, Category = "Animation|Falling")
	bool IsLandingRecoveryActive() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Falling")
	bool ShouldPlayHeavyLanding() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Falling")
	bool IsLandingRecoveryInputLocked() const;

	UFUNCTION(BlueprintCallable, Category = "Animation|Falling")
	void CompleteLandingRecoveryAnimation();

	bool ResolveLandingRecoveryBeforeAction(EActionCommand Command, EActionDirection Direction = EActionDirection::Any);

	UFUNCTION(BlueprintPure, Category = "Animation|Falling")
	float GetLastFallDistance() const;

	UFUNCTION(BlueprintPure, Category = "Movement|Falling")
	bool IsFallDamageSuppressed() const;

	UFUNCTION(BlueprintCallable, Category = "Movement|Falling")
	void SetFallDamageSuppressed(bool bSuppressed, FName Source);

	UFUNCTION(BlueprintCallable, Category = "Movement|Falling")
	void ClearFallDamageSuppression();

	bool RequestKnockDownGetUpEscape();
	bool RequestKnockDownGetUpDodgeEscape(EActionDirection DodgeDirection);
	void SetKnockDownGetUpDodgeInputHeld(bool bHeld, EActionDirection DodgeDirection);

	UFUNCTION(BlueprintCallable, Category = "Combat|Death")
	void DropWeaponForDeath();

	UFUNCTION(BlueprintPure, Category = "Combat|Death")
	bool CanDropWeaponForDeath() const;

	// 다운/에어본 사망처럼 바닥에 완전히 누운 뒤 죽음 처리를 마무리해야 할 때 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Combat|Death")
	void FinishDeferredDeath(EActionDirection DeathDirection = EActionDirection::Any);

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

	/** 사망 시 드롭된 무기를 다시 손에 부착 */
	void ResetWeaponAttachment();

protected:
	// CharacterBase 훅
	virtual void PostInitializeComponents() override;
	virtual void Falling() override;
	virtual void Landed(const FHitResult& Hit) override;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	TObjectPtr<UCameraOcclusionFadeComponent> CameraOcclusionFadeComponent;

	// SpringArm 충돌은 벽만 안정적으로 밀어내고 근접 전투 대상에는 덜 민감하게 둔다.
	UPROPERTY(EditAnywhere, Category = "Camera|Collision")
	bool bEnableCameraCollision = true;

	UPROPERTY(EditAnywhere, Category = "Camera|Collision", meta = (ClampMin = "0.0", Units = "cm"))
	float CameraProbeSize = 8.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Collision")
	TEnumAsByte<ECollisionChannel> CameraProbeChannel = ECC_Camera;

	// 플레이어 이동을 따라가는 카메라 위치 보간 사용 여부
	UPROPERTY(EditAnywhere, Category = "Camera|Lag")
	bool bEnableCameraLag = true;

	UPROPERTY(EditAnywhere, Category = "Camera|Lag", meta = (ClampMin = "0.0"))
	float CameraLagSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Lag", meta = (ClampMin = "0.0", Units = "cm"))
	float CameraLagMaxDistance = 80.f;

	// 카메라 회전 입력의 급격한 전환을 SpringArm에서 한 번 더 완화한다.
	UPROPERTY(EditAnywhere, Category = "Camera|Lag")
	bool bEnableCameraRotationLag = true;

	UPROPERTY(EditAnywhere, Category = "Camera|Lag", meta = (ClampMin = "0.0"))
	float CameraRotationLagSpeed = 12.f;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	TObjectPtr<UPlayerWeaponVFX> PlayerWeaponVFX;

	FVector DefaultWeaponRelativeLocation;
	FRotator DefaultWeaponRelativeRotation;

	// 스탯 및 액션 콜백
	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);

	UFUNCTION()
	void OnStaminaChanged(float CurrentStamina, float MaxStamina);

	void BindActionCallbacks();
	void BindGuardActionCallbacks();
	void BindDodgeActionCallbacks();
	void BindLockOnTargetCallbacks();
	void InitializeCameraDefaults();
	void ApplyCameraCollisionSettings() const;
	void ApplyCameraLagSettings() const;
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

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Death", meta = (DisplayName = "OnDeathMontageStarted"))
	void K2_OnDeathMontageStarted(EActionDirection DeathDirection, UAnimMontage* DeathMontage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Death", meta = (DisplayName = "OnDeathMontageEnded"))
	void K2_OnDeathMontageEnded(bool bInterrupted);

	UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Falling", meta = (DisplayName = "OnFallStarted"))
	void K2_OnFallStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Falling", meta = (DisplayName = "OnLandedFromFall"))
	void K2_OnLandedFromFall(float FallDistance, float AppliedDamage, bool bFatalFall);

	UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Falling", meta = (DisplayName = "OnLandingRecoveryStarted"))
	void K2_OnLandingRecoveryStarted(float FallDistance, UAnimMontage* LandingMontage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Falling", meta = (DisplayName = "OnLandingRecoveryEnded"))
	void K2_OnLandingRecoveryEnded();
	
	// 공격 관련
	const float WeaponRadius = 20.f; // 충돌 판정 시 검 두께
	int32 NowComboTransitionTid = 0; // 다음 콤보 결정할 때 사용
	int32 NextComboTransitionTid = 0; // 결정된 다음 콤보 저장
	
	UPROPERTY()
	TObjectPtr<UAnimMontage> NextAttackMontage = nullptr;
	EActionType NextAttackActionType = EActionType::None;
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage = nullptr;
	int32 ActiveAttackPlaybackId = 0;
	int32 NextAttackPlaybackId = 1;
	
	// 최대 차징 시간
	UPROPERTY(EditAnywhere, Category="Combat")
	float MaxChargeTime = 1.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FTimerHandle ChargeAttackTimerHandle;
	
private:
	// 공격 런타임
	void ClearAttackRuntimeState();
	bool IsActiveAttackMontagePlaying() const;
	int64 MakeComboOverrideKey(int32 NowComboTid, EActionCommand ActionCommand) const;
	
	void BindAttackCallbacks();
	void HandleAttackDamageResolved(AActor* Victim, const FHitResult& HitResult, float AppliedDamage);
	bool ShouldTriggerAttackHitStop(const AActor* Victim, float AppliedDamage) const;
	void StartAttackHitStop();
	void FinishAttackHitStop(int32 PlaybackId);
	void ClearAttackHitStop(bool bResumePausedMontage);

	// 이동 런타임
	void ResolveInitialGroundedMovementMode();
	void TickMovementRuntime(float DeltaTime);
	void UpdatePhaseFromInputAndGait(float DeltaTime);
	void BeginMovementPhase(EPlayerMovementPhase NewPhase);
	void FinishCurrentMovementPhase();
	void SetActiveGaitAndSpeed(EMovementState NewGait);
	void UpdateMaxWalkSpeed(float DeltaTime);
	EMovementState GetMovementAllowedGait(EMovementState RequestedGait) const;
	bool ShouldUseAnalogWalkGait(EMovementState RequestedGait) const;
	float GetAnalogWalkInputThreshold() const;
	float GetMoveInputScaleForActiveGait() const;
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
	void StopGuardImmediately();
	void ScheduleGuardReleaseGrace(float OverrideDelay = -1.f);
	void ClearGuardReleaseGrace();
	bool ShouldDelayGuardRelease() const;
	bool CanUseGuardReleaseGraceAfterSuccess() const;
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

	// 낙하/착지
	bool ShouldTrackFall() const;
	void BeginFallTracking();
	void EndFallTrackingFromLanding();
	float CalculateFallDamage(float FallDistance) const;
	float ApplyFallDamage(float FallDistance, bool& bOutFatalFall);
	void BeginLandingRecovery(float FallDistance);
	void FinishLandingRecovery();
	void ResetLandingRecovery();
	void PlayLandingRecoveryCameraShake();
	void EndLandingRecovery(bool bStartQueuedAction);
	void QueueLandingRecoveryAction(EActionCommand Command, EActionDirection Direction);
	void ClearQueuedLandingRecoveryAction();
	void StartQueuedLandingRecoveryAction(EActionCommand Command, EActionDirection Direction);

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
	const TMap<EActionDirection, TObjectPtr<UAnimMontage>>* GetDeathMontageMapForDamageReaction(
		EBADamageReactionType DamageReactionType) const;
	UAnimMontage* SelectDeathMontage(EActionDirection HitDirection) const;
	void DropWeaponAndDie();
	bool ShouldDeferDeathUntilDamageReaction() const;
	bool ShouldDeferMovementDisableForDeathMontage() const;
	void StartDeferredDamageReactionDeath();
	void FinalizeDeferredDamageReactionDeath();
	void FinalizeDropWeaponAndDie(EActionDirection DeathDirection);
	void FinalizeDeathAfterMontage();
	void PrepareDeathState();
	void StopMontagesForDeath();
	void ApplyDeathMontageRootMotionMode();
	void RestoreDeathMontageRootMotionMode();
	bool ShouldDropWeaponOnDeath() const;
	bool ShouldDropWeaponImmediatelyOnDeath() const;
	void DetachWeaponForDeath();
	void ConfigureDroppedWeaponCollision();
	void ConfigureDroppedWeaponWeight();
	void ApplyDroppedWeaponPhysics();
	FVector CalculateDeathWeaponDropImpulse() const;
	void PlayDeathMontage(EActionDirection DeathDirection);
	void FreezeMontageAtFinalFrame(UAnimMontage* MontageToPause);
	void HandleDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void HandleDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void HandleGuardHitReactionMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 PlaybackId);
	void HandleDeferredDeathReactionMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 PlaybackId);
	void HandleKnockDownReactionMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 PlaybackId);
	void ResetKnockDownRecovery();
	void ScheduleKnockDownRecoveryStart(int32 PlaybackId, float ReactionDuration);
	void TryBeginKnockDownRecoveryWait(int32 PlaybackId);
	void BeginKnockDownRecoveryWait();
	bool TryStartKnockDownGetUpEscape(bool bQueueDodge, EActionDirection DodgeDirection);
	void OpenKnockDownGetUpEscapeWindow();
	void CloseKnockDownGetUpEscapeWindow();
	void EscapeKnockDownGetUpImmediately();
	void RefreshKnockDownGetUpForMoveInput();
	void StartKnockDownGetUp();
	void FinishKnockDownGetUp();
	void HandleKnockDownGetUpMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void ApplyDamageReactionKnockback(
		EBADamageReactionType DamageReactionType,
		const FVector& DamageDirection,
		EActionDirection HitDirection,
		bool bGuarding,
		bool bGuardBreak);
	void ApplyGroundDamageReactionKnockback(const FVector& KnockbackDirection, float KnockbackStrength);
	void PlayDamageReactionCameraShake(
		EBADamageReactionType DamageReactionType,
		bool bGuarding,
		bool bGuardBreak);
	void PlayPerfectGuardCameraShake();
	void PlayDamageReactionForceFeedback(
		EBADamageReactionType DamageReactionType,
		bool bGuarding,
		bool bGuardBreak) const;
	void PlayPerfectGuardForceFeedback() const;
	void PlayConfiguredForceFeedback(float Intensity, float Duration) const;
	void PlayDeathCameraShake();
	void PlayConfiguredCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale) const;
	float ResolveDamageReactionCameraShakeScale(
		EBADamageReactionType DamageReactionType,
		bool bGuarding,
		bool bGuardBreak) const;
	void FinishDamageReaction(int32 PlaybackId);

	// 공격/피격 후딜 탈출
	bool IsAttackRecoveryEscapeState() const;
	bool IsDamageReactionRecoveryEscapeState() const;
	bool CanUseRecoveryEscapeDodge() const;
	bool CanUseRecoveryEscapeGuard() const;
	bool CanUseRecoveryEscapeMove() const;
	void ClearRecoveryEscapeWindow();
	void ExitCurrentRecoveryForEscape(bool bKeepQueuedAttack);
	void ExitAttackRecoveryForEscape(bool bKeepQueuedAttack);
	void ExitDamageReactionRecoveryForEscape();

	UFUNCTION()
	void HandleRespawnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	// 스킬 컴포넌트에서 런타임에 커맨드를 오버라이드하기 위한 값
	// 키: MakeComboOverrideKey(NowComboTransitionTid, InActionCommand), 값: NextComboTransitionTid
	UPROPERTY(VisibleAnywhere)
	TMap<int64, int32> ComboTransitionOverrides;

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

	// 낙하 피해 시작 높이
	UPROPERTY(EditAnywhere, Category = "Movement|Falling", meta = (ClampMin = "0.0", Units = "cm"))
	float SafeFallDistance = 400.f;

	// 낙사 높이
	UPROPERTY(EditAnywhere, Category = "Movement|Falling", meta = (ClampMin = "0.0", Units = "cm"))
	float FatalFallDistance = 1500.f;

	// 피해 시작점 현재 HP 비율
	UPROPERTY(EditAnywhere, Category = "Movement|Falling", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "%"))
	float FallDamageMinCurrentHPPercent = 15.f;

	// 낙사 직전 현재 HP 비율
	UPROPERTY(EditAnywhere, Category = "Movement|Falling", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "%"))
	float FallDamageMaxCurrentHPPercent = 80.f;

	// 약착지 입력 잠금 시작 높이
	UPROPERTY(EditAnywhere, Category = "Movement|Falling|Recovery", meta = (ClampMin = "0.0", Units = "cm"))
	float LandingInputLockMinFallDistance = 100.f;

	// 강착지 판정 시작 높이
	UPROPERTY(EditAnywhere, Category = "Movement|Falling|Recovery", meta = (ClampMin = "0.0", Units = "cm"))
	float LandingRecoveryMinFallDistance = 900.f;

	// 착지 잠금 자동 종료 시간
	UPROPERTY(EditAnywhere, Category = "Movement|Falling|Recovery", meta = (ClampMin = "0.0", Units = "s"))
	float LandingRecoveryAutoFinishDuration = 2.f;

	// 강착지 카메라 셰이크 클래스
	UPROPERTY(EditAnywhere, Category = "Movement|Falling|Recovery|Camera")
	TSubclassOf<UCameraShakeBase> LandingRecoveryCameraShakeClass;

	// 강착지 카메라 셰이크 강도 배율
	UPROPERTY(EditAnywhere, Category = "Movement|Falling|Recovery|Camera", meta = (ClampMin = "0.0"))
	float LandingRecoveryCameraShakeScale = 15.f;

	// 락온 성공 시 Strafe 고정 여부
	UPROPERTY(EditAnywhere, Category = "LockOn|Movement")
	bool bForceStrafeWhileLockedOn = true;

	// 카메라 상대 Pitch 기준 락온 PitchOffset 자동 보정 여부
	UPROPERTY(EditAnywhere, Category = "LockOn|Camera")
	bool bAutoCalibrateLockOnControllerPitch = true;

	// 락온 타겟 화면 높이 보정 PitchOffset
	UPROPERTY(EditAnywhere, Category = "LockOn|Camera", meta = (Units = "deg"))
	float LockOnAdditionalControllerPitchOffset = 0.f;

	// 근접 락온 시 카메라가 과하게 땅을 향하지 않도록 컨트롤러 pitch를 제한한다.
	UPROPERTY(EditAnywhere, Category = "LockOn|Camera")
	bool bOverrideLockOnControllerPitchClamp = true;

	UPROPERTY(EditAnywhere, Category = "LockOn|Camera", meta = (Units = "deg", EditCondition = "bOverrideLockOnControllerPitchClamp"))
	FVector2D LockOnControllerPitchClamp = FVector2D(-25.f, 30.f);

	// 락온 카메라 회전 보간 속도. 낮을수록 타겟을 더 부드럽게 따라간다.
	UPROPERTY(EditAnywhere, Category = "LockOn|Camera", meta = (ClampMin = "0.0"))
	float LockOnControllerRotationInterpSpeed = 10.f;

	// 공격 성공 시 플레이어 공격 몽타주 정지 시간
	UPROPERTY(EditAnywhere, Category = "Combat|Attack|HitStop", meta = (ClampMin = "0.0", Units = "s"))
	float AttackHitStopDuration = 0.1f;

	// 피격 반응 설정
	// 가드 정면 판정 좌우 허용 각도
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float GuardDamageBlockAngle = 120.f;

	// 피격 몽타주 없음/재생 실패 시 기본 리액션 시간
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction")
	float DamageReactionFallbackDuration = 0.6f;

	// 일반 피격 수평 넉백 세기
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float HitReactKnockbackStrength = 500.f;

	// 큰 피격 수평 넉백 세기
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float LargeHitReactKnockbackStrength = 500.f;

	// KnockDown Launch 없음 fallback 수평 넉백 세기
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float KnockDownKnockbackStrength = 650.f;

	// KnockDown Launch 없음 fallback 수직 런치 세기
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float KnockDownLaunchVerticalSpeed = 260.f;

	// 일반 가드 성공 수평 넉백 세기
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback")
	float GuardHitKnockbackStrength = 360.f;

	// 일반 피격/가드 수평 넉백 유지 시간
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Knockback", meta = (ClampMin = "0.0", Units = "s"))
	float GroundDamageReactionKnockbackDuration = 0.12f;

	// 후딜 탈출 시 현재 공격/피격 몽타주를 정리하는 blend-out 시간
	UPROPERTY(EditAnywhere, Category = "Combat|Recovery", meta = (ClampMin = "0.0", Units = "s"))
	float RecoveryEscapeMontageBlendOut = 0.05f;

	// 일반 피격 방향별 리액션 몽타주
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> HitReactMontages;

	// 큰 피격 방향별 리액션 몽타주
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> LargeHitReactMontages;

	// KnockDown/Airborne 방향별 리액션 몽타주
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> KnockDownReactMontages;

	// 가드 성공 방향별 짧은 리액션 몽타주
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> GuardHitReactMontages;

	// 가드 브레이크 방향별 긴 리액션 몽타주
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> GuardBreakReactMontages;

	// KnockDown 일반 기립/이동 탈출 기립 몽타주
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Recovery")
	TObjectPtr<UAnimMontage> KnockDownGetUpMontage;

	// 누운 자세 고정 최소 리액션 몽타주 시간
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Recovery", meta = (ClampMin = "0.0"))
	float KnockDownRecoveryMinMontageTime = 1.3f;

	// 누운 자세 고정 후 입력 탈출 창 시작 시간
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Recovery", meta = (ClampMin = "0.0"))
	float KnockDownGetUpEscapeInputStartDelay = 0.1f;

	// 누운 자세 고정 후 입력 탈출 창 종료 시간
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Recovery", meta = (ClampMin = "0.0"))
	float KnockDownGetUpEscapeInputEndDelay = 2.f;

	// 입력 탈출 창 종료 후 자동 기립 추가 대기 시간
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Recovery", meta = (ClampMin = "0.0"))
	float KnockDownGetUpNoInputDelayAfterEscapeWindow = 0.5f;

	// 이동 탈출 시 재생할 기립 몽타주 비율
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Recovery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float KnockDownGetUpMoveInputMontageFraction = 0.5f;

	// 이동 탈출 기립 몽타주 중단 blend-out 시간
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Recovery", meta = (ClampMin = "0.0"))
	float KnockDownGetUpStopBlendOut = 0.1f;

	// 피격 카메라 셰이크 클래스
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Camera")
	TSubclassOf<UCameraShakeBase> DamageReactionCameraShakeClass;

	// 피격/가드 카메라 셰이크 전체 내부 배율
	float DamageReactionCameraShakeScale = 1.f;

	// 일반 피격 카메라 셰이크 강도 배율
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Camera", meta = (ClampMin = "0.0"))
	float HitReactCameraShakeScale = 0.15f;

	// 큰 피격 카메라 셰이크 강도 배율
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Camera", meta = (ClampMin = "0.0"))
	float LargeHitReactCameraShakeScale = 1.f;

	// 넉다운/에어본 카메라 셰이크 강도 배율
	UPROPERTY(EditAnywhere, Category = "Combat|DamageReaction|Camera", meta = (ClampMin = "0.0"))
	float KnockDownCameraShakeScale = 1.75f;

	// 일반 가드 카메라 셰이크 강도 배율
	UPROPERTY(EditAnywhere, Category = "Combat|Guard|Camera", meta = (ClampMin = "0.0"))
	float GuardHitCameraShakeScale = 0.7f;

	// 퍼펙트 가드 카메라 셰이크 강도 배율
	UPROPERTY(EditAnywhere, Category = "Combat|Guard|Camera", meta = (ClampMin = "0.0"))
	float PerfectGuardCameraShakeScale = 1.25f;

	// 가드 브레이크 카메라 셰이크 강도 배율
	UPROPERTY(EditAnywhere, Category = "Combat|Guard|Camera", meta = (ClampMin = "0.0"))
	float GuardBreakCameraShakeScale = 1.25f;

	// 피격/가드 시 게임패드 Force Feedback 사용 여부
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback")
	bool bEnableGamepadForceFeedback = true;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitReactForceFeedbackIntensity = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", Units = "s"))
	float HitReactForceFeedbackDuration = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LargeHitReactForceFeedbackIntensity = 0.55f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", Units = "s"))
	float LargeHitReactForceFeedbackDuration = 0.16f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float KnockDownForceFeedbackIntensity = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", Units = "s"))
	float KnockDownForceFeedbackDuration = 0.22f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GuardHitForceFeedbackIntensity = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", Units = "s"))
	float GuardHitForceFeedbackDuration = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PerfectGuardForceFeedbackIntensity = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", Units = "s"))
	float PerfectGuardForceFeedbackDuration = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GuardBreakForceFeedbackIntensity = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Combat|Feedback|ForceFeedback", meta = (ClampMin = "0.0", Units = "s"))
	float GuardBreakForceFeedbackDuration = 0.2f;

	// 일반 피격 사망 방향별 몽타주
	UPROPERTY(EditAnywhere, Category = "Combat|Death|Montage", meta = (DisplayName = "Hit React Death Montages"))
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> DeathMontages;

	// 큰 피격 사망 방향별 몽타주
	UPROPERTY(EditAnywhere, Category = "Combat|Death|Montage")
	TMap<EActionDirection, TObjectPtr<UAnimMontage>> LargeHitDeathMontages;

	// 사망 몽타주 중단/마지막 자세 고정 전 blend-out 시간
	UPROPERTY(EditAnywhere, Category = "Combat|Death|Montage", meta = (ClampMin = "0.0"))
	float DeathMontageStopBlendOut = 0.1f;

	// 사망 카메라 셰이크 클래스
	UPROPERTY(EditAnywhere, Category = "Combat|Death|Camera")
	TSubclassOf<UCameraShakeBase> DeathCameraShakeClass;

	// 사망 카메라 셰이크 강도 배율
	UPROPERTY(EditAnywhere, Category = "Combat|Death|Camera", meta = (ClampMin = "0.0"))
	float DeathCameraShakeScale = 1.8f;

	// 사망 시 장착 무기 드랍 여부
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop")
	bool bDropWeaponOnDeath = true;

	// 드랍 무기 물리 질량
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop", meta = (ClampMin = "0.0"))
	float DroppedWeaponMassKg = 35.f;

	// 드랍 무기 직선 이동 감쇠
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop", meta = (ClampMin = "0.0"))
	float DroppedWeaponLinearDamping = 2.f;

	// 드랍 무기 회전 감쇠
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop", meta = (ClampMin = "0.0"))
	float DroppedWeaponAngularDamping = 12.f;

	// 드랍 무기 최대 회전 속도 제한
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop", meta = (ClampMin = "0.0"))
	float DroppedWeaponMaxAngularSpeedDeg = 180.f;

	// 사망 시 무기 전방 impulse
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop")
	float DroppedWeaponForwardImpulse = 0.f;

	// 사망 시 무기 우측 impulse
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop")
	float DroppedWeaponRightImpulse = 0.f;

	// 사망 시 무기 상향 impulse
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop")
	float DroppedWeaponUpwardImpulse = 0.f;

	// 사망 시 무기 회전 impulse
	UPROPERTY(EditAnywhere, Category = "Combat|Death|WeaponDrop")
	FVector DroppedWeaponAngularImpulse = FVector::ZeroVector;

	// 가드 몽타주 반복 섹션 이름
	UPROPERTY(EditAnywhere, Category = "Combat|Guard|Montage")
	FName GuardLoopSection = TEXT("Loop");

	// 가드 입력을 뗀 뒤에도 방어 자세를 유지하는 시간
	UPROPERTY(EditAnywhere, Category = "Combat|Guard", meta = (ClampMin = "0.0", Units = "s"))
	float GuardReleaseGraceDuration = 1.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Respawn|Montage")
	TObjectPtr<UAnimMontage> RespawnMontage;

	// 런타임 상태
	FBAPlayerMovementRuntimeState MovementRuntime;
	FBAPlayerSprintRuntimeState SprintRuntime;
	FBAPlayerLadderRuntimeState LadderRuntime;
	EPlayerLocomotionMode LocomotionModeBeforeLockOn = EPlayerLocomotionMode::Free;
	EPlayerDamageReactionState DamageReactionState = EPlayerDamageReactionState::None;
	FTimerHandle DamageReactionTimerHandle;
	FTimerHandle KnockDownRecoveryStartTimerHandle;
	FTimerHandle KnockDownGetUpTimerHandle;
	FTimerHandle LandingRecoveryTimerHandle;
	float FallStartZ = 0.f;
	float LastFallDistance = 0.f;
	TSet<FName> FallDamageSuppressionSources;
	EActionCommand LandingRecoveryQueuedCommand = EActionCommand::None;
	EActionDirection LandingRecoveryQueuedDirection = EActionDirection::Any;
	int32 ActiveDamageReactionPlaybackId = 0;
	int32 NextDamageReactionPlaybackId = 1;
	EActionDirection LastDamageHitDirection = EActionDirection::Any;
	EBADamageReactionType LastDamageReactionType = EBADamageReactionType::HitReact;
	FVector LastDamageDirection = FVector::ZeroVector;
	float LastDamageLaunchHorizontalSpeed = 0.f;
	float LastDamageLaunchVerticalSpeed = 0.f;
	bool bDeathFinalizationDeferred = false;
	bool bDeathMovementDisableDeferred = false;
	bool bWeaponDroppedForDeath = false;
	bool bGuardInputHeld = false;
	bool bGuardReleaseGraceAvailable = false;
	bool bPerfectGuardWindowActive = false;
	FTimerHandle GuardReleaseGraceTimerHandle;
	bool bFallTrackingActive = false;
	bool bLandingRecoveryActive = false;
	bool bLandingRecoveryInputLocked = false;
	bool bLockOnForcedStrafeActive = false;
	bool bPendingLockOnStrafeAfterDodge = false;
	bool bKnockDownWaitingForGetUp = false;
	bool bKnockDownGetUpInProgress = false;
	bool bKnockDownGetUpEscapeWindowOpen = false;
	bool bKnockDownGetUpQueuedByMoveInput = false;
	bool bKnockDownGetUpQueuedDodgeInput = false;
	bool bKnockDownGetUpDodgeInputHeld = false;
	bool bKnockDownGetUpMoveInputShortcut = false;
	EActionDirection KnockDownGetUpQueuedDodgeDirection = EActionDirection::Any;
	EActionDirection KnockDownGetUpHeldDodgeDirection = EActionDirection::Any;
	int32 RecoveryEscapeWindowCount = 0;
	int32 RecoveryEscapeDodgeWindowCount = 0;
	int32 RecoveryEscapeGuardWindowCount = 0;
	int32 RecoveryEscapeMoveWindowCount = 0;
	UPROPERTY(VisibleAnywhere) bool bIsBeforeCharge = false;
	UPROPERTY(VisibleAnywhere) bool bIsCharging = false;
	UPROPERTY(VisibleAnywhere) bool bIsChargeInputCompleted = false;
	UPROPERTY(Transient) UAnimMontage* PausedMontage = nullptr;
	UPROPERTY(Transient) UAnimMontage* HitStopPausedMontage = nullptr;
	FTimerHandle AttackHitStopTimerHandle;
	int32 AttackHitStopPlaybackId = 0;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDamageReactionMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveKnockDownGetUpMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDeathMontage;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAnimInstance> DeathRootMotionModeAnimInstance;

	TEnumAsByte<ERootMotionMode::Type> PreviousDeathRootMotionMode = ERootMotionMode::NoRootMotionExtraction;
	bool bDeathRootMotionModeOverridden = false;
};
