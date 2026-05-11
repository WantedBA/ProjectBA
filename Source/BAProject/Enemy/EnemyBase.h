#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "EnemyBase.generated.h"

class UStateComponent;
class UStatComponent;
class UCombatComponent;

UCLASS(Abstract)
class BAPROJECT_API AEnemyBase : public ACharacterBase
{
	GENERATED_BODY()

public:
	AEnemyBase();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void InitializeFromTable(int32 InTid);

protected:
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;

	virtual void OnDamaged(float FinalDamage, AActor* DamageCauser) override;
	virtual void OnDeath() override;

	// 시각 연출 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Visuals", meta = (DisplayName = "OnHitVisuals"))
	void K2_OnHitVisuals(FVector HitLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Visuals", meta = (DisplayName = "OnDeadVisuals"))
	void K2_OnDeadVisuals();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStateComponent> StateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	int32 MonsterTid;
};
