#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Player/BAPlayerCharacterTypes.h"
#include "BAPlayerCharacter.generated.h"

class UCameraComponent;
class UInteractorComponent;
class USpringArmComponent;
class UStatComponent;

UCLASS()
class BAPROJECT_API ABAPlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()
	
public:
	// Lifecycle
	ABAPlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Attack() override;
	
	UFUNCTION(BlueprintCallable, Category = Initialization)
	virtual void InitializeFromTable();

	// Input and state setters
	void SetMovementState(EMovementState NewState);
	void SetMoveInputVector(const FVector2D& NewMoveInput);
	void SetHasMoveInput(bool bNewHasMoveInput);

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void SetLocomotionMode(EPlayerLocomotionMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void SetCombatMode(EPlayerCombatMode NewMode);

	// Animation query API
	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	EMovementState GetMovementState() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	EPlayerLocomotionMode GetLocomotionMode() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Combat")
	EPlayerCombatMode GetCombatMode() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool HasMoveInput() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	FVector2D GetMoveInputVector() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	FVector GetMoveInputWorldDirection() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	FVector GetMoveInputLocalDirection() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	float GetMoveInputDirectionAngle() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	float GetVelocityDirectionAngle() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	float GetGroundSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool IsSprintLockedAfterExhausted() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool IsSprintEntryRotationLocked() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	float GetSprintTurnDeltaAngle() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	float GetTurnaroundToControlRotationAngle() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	float GetTurnaroundPlayRate() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool IsSprintStopRequested() const;

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void ClearSprintStopRequest();

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void CompleteSprintStopAnimation();

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool IsTurnaroundRequested() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool IsTurnaroundQueuedAfterSprintStop() const;

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void BeginTurnaroundAnimation();

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void CompleteTurnaroundAnimation();

	// Interaction API
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
	// Locomotion internals
	void ApplyLocomotionMovementPolicy();
	void ApplyFreeMovementPolicy(class UCharacterMovementComponent& MovementComponent);
	void ApplyStrafeMovementPolicy(class UCharacterMovementComponent& MovementComponent);
	void ApplyTurnaroundMovementPolicy(class UCharacterMovementComponent& MovementComponent);
	void UpdateSmoothedMoveInput(float DeltaTime);
	FVector2D GetMoveInputDirectionVector() const;
	FVector2D GetMoveInputVectorFromWorldDirection(const FVector& WorldDirection) const;
	bool ShouldUseSprintMovementPolicy() const;

	// Sprint internals
	bool CanSprint() const;
	bool IsSprintMovementActive() const;
	void ConsumeSprintStamina(float DeltaTime);
	float CalculateSprintStaminaCost(float DeltaTime) const;
	void LockSprintIfExhausted();
	void UpdateSprintExhaustionLock();
	void UpdateSprintEntryRotation(float DeltaTime);
	bool ShouldKeepStrafeRotationDuringSprintEntry() const;
	void RequestSprintStop();
	void UpdateSprintStopRequest(float DeltaTime);
	void StartSprintStopRequestWindow();
	void UpdateSprintStopRequestWindow(float DeltaTime);
	bool CanRequestSprintStop() const;

	// Turnaround internals
	void UpdateTurnaroundRotation(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ShowOnlyInnerProperties))
	FBAPlayerMovementSpeedSettings SpeedSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Locomotion", meta = (ShowOnlyInnerProperties))
	FBAPlayerLocomotionSettings LocomotionSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Sprint", meta = (ShowOnlyInnerProperties))
	FBAPlayerSprintSettings SprintSettings;

	UPROPERTY(EditAnywhere, Category = "Movement|Turnaround", meta = (ShowOnlyInnerProperties))
	FBAPlayerTurnaroundSettings TurnaroundSettings;

	FBAPlayerMovementRuntimeState MovementRuntime;
	FBAPlayerSprintRuntimeState SprintRuntime;
	FBAPlayerTurnaroundRuntimeState TurnaroundRuntime;

protected:
	// virtual	void SetupHUDWidget(class UHUDWidget* InHUDWidget) override;

	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);
	UFUNCTION()
	void OnStaminaChanged(float CurrentStamina, float MaxStamina);
};
