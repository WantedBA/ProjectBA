// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Character/CharacterBase.h"
#include "PlayerCharacter.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class BAPROJECT_API APlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()
	
public:
	APlayerCharacter();
	
protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	virtual void Attack() override;
	
protected:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	TObjectPtr<class UCameraComponent> Camera;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Input, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(VisibleAnywhere, Category = Input, BlueprintReadOnly)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(VisibleAnywhere, Category = Input, BlueprintReadOnly)
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(VisibleAnywhere, Category = Input, BlueprintReadOnly)
	TObjectPtr<UInputAction> AttackAction;

protected: // TODO: 은성님과 HUD 협업, HUDInterface 구현 필요
	// virtual	void SetupHUDWidget(class UHUDWidget* InHUDWidget) override;
};
