#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BAPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

UCLASS()
class BAPROJECT_API ABAPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABAPlayerController();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
private:
	// TODO: KM/Gamepad IMC 나누기
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RunAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LightAttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	// Input handlers
	void Move(const FInputActionValue& Value);
	void OnMoveCompleted();
	void Look(const FInputActionValue& Value);
	void LightAttack();
	void OnWalkStarted();
	void OnWalkCompleted();
	void OnSprintStarted();
	void OnSprintCompleted();
	void ApplyMovementStateByModifier() const;

	bool bWalkModifierHeld = false;
	bool bSprintModifierHeld = false;
	bool bHasMoveInput = false;
	
// protected: TODO: 은성님 HUD 작업
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
// 	TSubClassOf<class UHUDWidget> HUDWidgetClass;
// 	
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
// 	TObjectPtr<class UHUDWidget> HUDWidget;
	
};
