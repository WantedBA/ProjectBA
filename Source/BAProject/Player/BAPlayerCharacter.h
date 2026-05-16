#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Player/BAPlayerCharacterTypes.h"
#include "BAPlayerCharacter.generated.h"

class UCameraComponent;
class UCharacterMovementComponent;
class UInteractorComponent;
class USpringArmComponent;
class UStatComponent;

UCLASS()
class BAPROJECT_API ABAPlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	ABAPlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Attack() override;

	UFUNCTION(BlueprintCallable, Category = Initialization)
	virtual void InitializeFromTable();

	void SetMovementState(EMovementState NewState);
	void SetMoveInputVector(const FVector2D& NewMoveInput);
	void SetHasMoveInput(bool bNewHasMoveInput);

	UFUNCTION(BlueprintCallable, Category = "Animation|Movement")
	void SetLocomotionMode(EPlayerLocomotionMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void SetCombatMode(EPlayerCombatMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EMovementState GetMovementState() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EMovementState GetDesiredMovementState() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EPlayerLocomotionMode GetLocomotionMode() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	EPlayerMovementPhase GetMovementPhase() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Combat")
	EPlayerCombatMode GetCombatMode() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	bool HasMoveInput() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	FVector2D GetMoveInputVector() const;

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

	UFUNCTION(BlueprintPure, Category = "Interaction")
	UInteractorComponent* GetInteractorComponent() const { return InteractorComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractorComponent> InteractorComponent;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<UCameraComponent> Camera;

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
	void SyncFreeStrafeFacingMode();
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
	void LockSprintUntilRecovered();
	void UnlockSprintAfterRecovery();

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ShowOnlyInnerProperties))
	FBAPlayerMovementSpeedSettings SpeedSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion", meta = (ShowOnlyInnerProperties))
	FBAPlayerLocomotionSettings LocomotionSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Gait", meta = (ShowOnlyInnerProperties))
	FBAPlayerMovementGaitSettings GaitSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Sprint", meta = (ShowOnlyInnerProperties))
	FBAPlayerSprintCostSettings SprintCostSettings;

	FBAPlayerMovementRuntimeState MovementRuntime;
	FBAPlayerSprintRuntimeState SprintRuntime;

protected:
	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);
	UFUNCTION()
	void OnStaminaChanged(float CurrentStamina, float MaxStamina);
};
