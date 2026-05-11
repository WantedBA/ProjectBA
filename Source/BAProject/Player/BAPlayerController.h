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
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LightAttackAction;

	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void LightAttack();
	
// protected: TODO: 은성님 HUD 작업
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
// 	TSubClassOf<class UHUDWidget> HUDWidgetClass;
// 	
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
// 	TObjectPtr<class UHUDWidget> HUDWidget;
	
};
