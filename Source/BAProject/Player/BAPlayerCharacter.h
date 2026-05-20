#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Player/BAPlayerCharacterTypes.h"
#include "Tables/ActionEnums.h"
#include "BAPlayerCharacter.generated.h"

class UCombatComponent;
class UPlayerSkillComponent;
class UCameraComponent;
class UCharacterMovementComponent;
class UActionComponent;
class UActionAnimationComponent;
class UInteractorComponent;
class AMapLadder;
class USpringArmComponent;
class UStatComponent;
class UStaticMeshComponent;
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

	// 테이블 초기화, 컴포넌트 이벤트 바인딩, 초기 이동 상태 동기화를 수행한다.
	virtual void BeginPlay() override;

	// 일반 이동 또는 사다리 이동 런타임을 매 프레임 갱신한다.
	virtual void Tick(float DeltaTime) override;

// 전투 관련
	// CharacterBase 공격 진입점. LightAttack
	virtual void Attack() override;
	
	void HeavyAttack();
	
	virtual class UStaticMeshComponent* GetWeaponMesh() const override { return WeaponMeshComponent; }

// --------------------
	// UserDataSubsystem과 ActionData 테이블을 읽어 스탯, 이동 속도, 질주 비용을 초기화한다.
	UFUNCTION(BlueprintCallable, Category = Initialization)
	virtual void InitializeFromTable();

	// 컨트롤러 입력이 요청한 보행/달리기/질주 상태를 설정한다.
	void SetMovementState(EMovementState NewState);

	// 컨트롤러 기준 2D 이동 입력을 저장하고 월드/로컬 방향 계산에 사용한다.
	void SetMoveInputVector(const FVector2D& NewMoveInput);

	// 현재 프레임에 유효한 이동 입력이 있는지 애니메이션 상태로 전달한다.
	void SetHasMoveInput(bool bNewHasMoveInput);

	// 자유 이동과 스트레이프 이동 모드를 전환한다.
	UFUNCTION(BlueprintCallable, Category = "Animation|Movement")
	void SetLocomotionMode(EPlayerLocomotionMode NewMode);

	// 애니메이션 블루프린트에서 사용할 전투 태세를 설정한다.
	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void SetCombatMode(EPlayerCombatMode NewMode);

	// 스태미너 제한까지 반영된 실제 활성 이동 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EMovementState GetMovementState() const;

	// 입력이 요청한 이동 상태를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EMovementState GetDesiredMovementState() const;

	// 현재 회전/가속 세팅에 적용된 이동 모드를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EPlayerLocomotionMode GetLocomotionMode() const;

	// Start, Loop, Stop, Turn 등 현재 재생해야 하는 이동 페이즈를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EPlayerMovementPhase GetMovementPhase() const;

	// 애니메이션 블루프린트용 전투 모드를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Combat")
	EPlayerCombatMode GetCombatMode() const;

	// 현재 이동 입력이 남아 있는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	bool HasMoveInput() const;

	// 원본 2D 이동 입력을 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	FVector2D GetMoveInputVector() const;

	// 입력 벡터를 컨트롤러 yaw 기준 월드 방향으로 변환해 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	FVector GetMoveInputWorldDirection() const;

	// 입력 벡터를 액터 로컬 기준 방향으로 변환해 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	FVector GetMoveInputLocalDirection() const;

	// 액터 정면과 입력 방향 사이의 yaw 각도를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetMoveInputDirectionAngle() const;

	// 액터 정면과 실제 속도 방향 사이의 yaw 각도를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetVelocityDirectionAngle() const;

	// 수평 이동 속도를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetGroundSpeed() const;

	// 현재 이동 페이즈 진입 시점의 방향 각도를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetMovementPhaseDirectionAngle() const;

	// 지정한 이동 페이즈가 현재 활성 상태인지 확인한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	bool IsMovementPhase(EPlayerMovementPhase Phase) const;

	// 현재 이동 페이즈가 루트 모션 애니메이션으로 처리되어야 하는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	bool IsMovementPhaseUsingRootMotion() const;

	// 애니메이션 블루프린트가 Start/Stop/Turn 몽타주 종료를 캐릭터에 통지할 때 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Animation|Movement")
	void CompleteMovementPhaseAnimation(EPlayerMovementPhase CompletedPhase);

	// 스태미너 고갈 후 재시작 기준까지 질주가 잠겨 있는지 반환한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Sprint")
	bool IsSprintLockedAfterExhausted() const;

	// 상호작용 후보 탐색과 실행을 담당하는 컴포넌트를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Interaction")
	UInteractorComponent* GetInteractorComponent() const { return InteractorComponent; }

	// 공용 액션 실행 상태와 입력 버퍼를 관리하는 컴포넌트를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	UActionComponent* GetActionComponent() const { return ActionComponent; }

	// 액션 몽타주와 액션 윈도우 판정을 담당하는 컴포넌트를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Action")
	UActionAnimationComponent* GetActionAnimationComponent() const { return ActionAnimationComponent; }

	// 사다리 관련 상호작용
	UFUNCTION(BlueprintCallable, Category = "Interaction|Ladder")
	void EnterLadder(AMapLadder* Ladder, const FVector& EntryLocation, const FRotator& FaceRotation);

	UFUNCTION(BlueprintCallable, Category = "Interaction|Ladder")
	void ExitLadder(const FVector& ExitLocation);

	UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
	bool IsOnLadder() const;

	// 현재 매달려 있는 사다리 액터를 반환한다. 사다리 상태가 아니면 nullptr이다.
	UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
	AMapLadder* GetCurrentLadder() const;

	// 입력, 질주 요청, 빠른 하강을 반영한 현재 사다리 수직 속도를 반환한다.
	UFUNCTION(BlueprintPure, Category = "Interaction|Ladder")
	float GetLadderClimbVelocity() const;

	// 컷신 시작 시 캐릭터 이동을 즉시 정지하고 이동 입력 처리를 잠근다.
	UFUNCTION(BlueprintCallable, Category = "Control|Cutscene")
	void LockMovementForCutscene();

	// 컷신 종료 후 일반 이동 입력 처리를 다시 허용한다.
	UFUNCTION(BlueprintCallable, Category = "Control|Cutscene")
	void UnlockMovementForCutscene();
	
protected:
	virtual void OnDamaged(float FinalDamage, AActor* DamageCauser) override;
	
private:
	void TickMovementRuntime(float DeltaTime);
	void UpdatePhaseFromInputAndGait(float DeltaTime);
	void BeginMovementPhase(EPlayerMovementPhase NewPhase);
	void FinishCurrentMovementPhase();
	void SetActiveGaitAndSpeed(EMovementState NewGait);
	EMovementState GetStaminaAllowedGait(EMovementState RequestedGait) const;
	const FBAPlayerMovementPhaseSettings& GetPhaseSettings(EMovementState Gait) const;
	float GetSpeedForGait(EMovementState Gait) const;
	bool IsPhaseEnabledForGait(EPlayerMovementPhase Phase, EMovementState Gait) const;
	bool DoesGaitPhaseUseRootMotion(EPlayerMovementPhase Phase, EMovementState Gait) const;
	bool ShouldEnterTurnPhase() const;
	float CalculateInputYawDeltaFromActor() const;

	void ApplyBufferedMoveInput();
	bool IsActionMovementLocked() const;
	void SyncFreeStrafeFacingMode();
	void UpdateInterpolatedFacingRotation();
	bool ShouldUseInterpolatedFacingRotation() const;
	void UseMovementDirectionFacing(UCharacterMovementComponent& MovementComponent);
	void UseControllerYawFacing(UCharacterMovementComponent& MovementComponent);
	void UpdateInterpolatedMoveInputDirection(float DeltaTime);
	void SnapInterpolatedMoveInputTo(const FVector2D& MoveInput);
	FVector2D GetInterpolatedMoveInputVector() const;
	FVector2D ConvertWorldDirectionToMoveInput(const FVector& WorldDirection) const;
	FVector ConvertMoveInputToWorldDirection(const FVector2D& MoveInput) const;

	bool IsSprintAllowedByStamina() const;
	void DrainSprintStaminaDuringLoop(float DeltaTime);
	float CalculateSprintStaminaDrain(float DeltaTime) const;
	void PauseSprintStaminaRecovery();
	void ResumeSprintStaminaRecovery(bool bApplyDelay);
	void LockSprintUntilRecovered();
	void UnlockSprintAfterRecovery();

	void TickLadderClimb(float DeltaTime);
	float CalculateLadderClimbSpeed(float VerticalInput) const;
	bool IsLadderSprintRequested() const;
	void DrainLadderSprintStamina(float DeltaTime);
	void ResetMovementRuntimeForLadder();

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

	FBAPlayerMovementRuntimeState MovementRuntime;
	FBAPlayerSprintRuntimeState SprintRuntime;
	FBAPlayerLadderRuntimeState LadderRuntime;

protected:
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
	
// 공격 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> AttackMontage;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	EBAPlayerState BAPlayerState = EBAPlayerState::None;
	
protected:
	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);
	UFUNCTION()
	void OnStaminaChanged(float CurrentStamina, float MaxStamina);
	UFUNCTION()
	void HandleActionStarted(int32 ActionTid, EActionType ActionType);
};
