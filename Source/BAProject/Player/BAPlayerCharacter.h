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
	void SetHasMoveInput(bool bNewHasMoveInput);

private:
	bool CanSprint() const;
	bool IsSprintMovementActive() const;
	void ConsumeSprintStamina(float DeltaTime);
	float CalculateSprintStaminaCost(float DeltaTime) const;
	void LockSprintIfExhausted();
	void UpdateSprintExhaustionLock();

	EMovementState CurrentMovementState = EMovementState::Run;
	
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

	void OnStaminaChanged(float CurrentStamina, float MaxStamina);

	virtual void BeginPlay() override;
};
