#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
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
	virtual void Attack() override;
	
	UFUNCTION(BlueprintCallable, Category = Initialization)
	virtual void InitializeFromTable();

	// Controller가 보행 상태를 변경할 수 있도록 공개
	void SetMovementState(EMovementState NewState);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStatComponent> StatComponent;
	
	
private:
	EMovementState CurrentMovementState = EMovementState::Run;
	
	UPROPERTY(EditAnywhere, Category="Movement")
	float WalkSpeed = 100.0f;
	
	UPROPERTY(EditAnywhere, Category="Movement")
	float RunSpeed = 400.0f;
	
	UPROPERTY(EditAnywhere, Category="Movement")
	float SprintSpeed = 700.0f;
	
protected:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class UCameraComponent> Camera;

protected: // TODO: 은성님과 HUD 협업, HUDInterface 구현 필요
	// virtual	void SetupHUDWidget(class UHUDWidget* InHUDWidget) override;
};
