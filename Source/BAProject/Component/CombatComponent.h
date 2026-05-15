#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimMontage;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	UFUNCTION(BlueprintCallable, Category = "Combat") // 몽타주 사용
	void ExecuteAttack(UAnimMontage* AttackMontage, float PlayRate = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckAnimNotifyHitStart(float InRadius, float InDamage, FName InSocketName = TEXT("Muzzle"));

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckAnimNotifyHitEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetWeaponCollisionActive(bool bActive);

protected:
	void CheckHit();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY()
	bool bIsHitChecking;

	UPROPERTY()
	float CurrentRadius;

	UPROPERTY()
	float CurrentDamage;

	UPROPERTY()
	FName CurrentSocketName;

	UPROPERTY()
	TArray<AActor*> HitActors;
};
