#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Tables/PlayerEnums.h"
#include "BAPlayerCharacter.generated.h"

class UInputAction;
class UInputMappingContext;

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Walk,
	Run,
	Sprint
};

UENUM(BlueprintType)
enum class EPlayerLocomotionMode : uint8
{
	Free,
	Strafe
};

UENUM(BlueprintType)
enum class EPlayerCombatMode : uint8
{
	None,
	Combat,
	Block
};

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
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStatComponent> StatComponent;
	
public:
	void SetMovementState(EMovementState NewState);
	void SetMoveInputVector(const FVector2D& NewMoveInput);
	void SetHasMoveInput(bool bNewHasMoveInput);

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void SetLocomotionMode(EPlayerLocomotionMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void SetCombatMode(EPlayerCombatMode NewMode);

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
	bool IsSprintStopRequested() const;

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void ClearSprintStopRequest();

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void CompleteSprintStopAnimation();

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool IsSprintTurnaroundRequested() const;

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void CompleteSprintTurnaroundAnimation();

private:
	bool CanSprint() const;
	bool IsSprintMovementActive() const;
	void ConsumeSprintStamina(float DeltaTime);
	float CalculateSprintStaminaCost(float DeltaTime) const;
	void LockSprintIfExhausted();
	void UpdateSprintExhaustionLock();
	void ApplyLocomotionMovementPolicy();
	void UpdateSprintEntryRotation(float DeltaTime);
	bool ShouldUseSprintEntryRotationLock() const;
	void RequestSprintStop();
	void UpdateSprintStopRequest(float DeltaTime);
	void StartSprintStopRequestWindow();
	void UpdateSprintStopRequestWindow(float DeltaTime);
	bool CanRequestSprintStop() const;
	bool ShouldUseSprintMovementPolicy() const;

	EMovementState CurrentMovementState = EMovementState::Run;
	EPlayerLocomotionMode CurrentLocomotionMode = EPlayerLocomotionMode::Free;
	EPlayerCombatMode CurrentCombatMode = EPlayerCombatMode::None;
	FVector2D MoveInputVector = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Movement|Locomotion")
	float FreeRotationRateYaw = 1440.f;

	UPROPERTY(EditAnywhere, Category="Movement|Locomotion")
	float FreeMaxAcceleration = 8192.f;

	UPROPERTY(EditAnywhere, Category="Movement|Locomotion")
	float FreeBrakingDecelerationWalking = 8192.f;

	UPROPERTY(EditAnywhere, Category="Movement|Locomotion")
	float FreeGroundFriction = 12.f;

	UPROPERTY(EditAnywhere, Category="Movement|Locomotion")
	float StrafeRotationRateYaw = 720.f;

	UPROPERTY(EditAnywhere, Category="Movement|Locomotion")
	float StrafeMaxAcceleration = 2048.f;

	UPROPERTY(EditAnywhere, Category="Movement|Locomotion")
	float StrafeBrakingDecelerationWalking = 2048.f;

	UPROPERTY(EditAnywhere, Category="Movement|Locomotion")
	float StrafeGroundFriction = 8.f;

	UPROPERTY(EditAnywhere, Category="Movement|Sprint")
	float SprintStrafeEntryBlendTime = 0.25f;

	UPROPERTY(EditAnywhere, Category="Movement|Sprint")
	float SprintStrafeEntryOrientationSpeedRatio = 0.85f;

	UPROPERTY(EditAnywhere, Category="Movement|Sprint")
	float SprintStopRequestHoldTime = 0.35f;

	UPROPERTY(EditAnywhere, Category="Movement|Sprint")
	float SprintStopMinSpeed = 150.f;

	UPROPERTY(EditAnywhere, Category="Movement|Sprint")
	float SprintStopRequestWindowTime = 0.2f;
	
	UPROPERTY(EditAnywhere, Category="Movement")
	float WalkSpeed = 100.0f;
	
	UPROPERTY(EditAnywhere, Category="Movement")
	float RunSpeed = 400.0f;
	
	UPROPERTY(EditAnywhere, Category="Movement")
	float SprintSpeed = 700.0f;

	float SprintStaminaCost = 0.f;
	EPlayerStaminaCostType SprintStaminaCostType = EPlayerStaminaCostType::Instant;
	float SprintMinRequiredStamina = 0.f;
	float SprintRestartStaminaPercent = 0.f;
	bool bHasSprintActionData = false;
	bool bHasMoveInput = false;
	bool bSprintLockedAfterExhausted = false;
	bool bSprintEntryRotationLocked = false;
	bool bSprintStopRequested = false;
	bool bSprintStopMovementLocked = false;
	bool bSprintStopStartedFromStrafe = false;
	bool bSprintTurnaroundRequested = false;
	bool bCanRequestSprintStopFromRecentExit = false;
	float SprintEntryElapsedTime = 0.f;
	float SprintStopRequestRemainingTime = 0.f;
	float SprintStopRequestWindowRemainingTime = 0.f;
	
protected:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class UCameraComponent> Camera;

protected:
	// virtual	void SetupHUDWidget(class UHUDWidget* InHUDWidget) override;

	UFUNCTION()
	void OnHealthChanged(float CurrentHP, float MaxHP);
	UFUNCTION()
	void OnStaminaChanged(float CurrentStamina, float MaxStamina);
};
