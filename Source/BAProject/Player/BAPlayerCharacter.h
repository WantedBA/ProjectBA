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
	Normal,
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

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	EMovementState GetMovementState() const;

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	EPlayerLocomotionMode GetLocomotionMode() const;

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

private:
	bool CanSprint() const;
	bool IsSprintMovementActive() const;
	void ConsumeSprintStamina(float DeltaTime);
	float CalculateSprintStaminaCost(float DeltaTime) const;
	void LockSprintIfExhausted();
	void UpdateSprintExhaustionLock();

	EMovementState CurrentMovementState = EMovementState::Run;
	EPlayerLocomotionMode CurrentLocomotionMode = EPlayerLocomotionMode::Normal;
	FVector2D MoveInputVector = FVector2D::ZeroVector;
	
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
