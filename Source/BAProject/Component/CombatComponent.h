#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitDetected, AActor*, Victim, const FHitResult&, HitResult);
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BAPROJECT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ExecuteAttack(UAnimMontage* AttackMontage, float PlayRate = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckHitStart(float InRadius, float InDamage, FName InSocketName = TEXT("Muzzle"));

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckHitEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetAttackData(float InRadius, float InDamage, FName InSocketName);

	void TriggerHitStop(float Duration);

	UFUNCTION(BlueprintCallable, Category = "Combat|VFX")
	void SpawnShockwave(FVector Location, float Scale = 1.0f);

	void CheckHitStartDefault();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ProcessHitCheck(); // 이전 프레임 소켓 위치와 현재 위치 사이를 스윕

	void ApplyDamage(AActor* Victim, const FHitResult& HitResult);

public:
	UPROPERTY(BlueprintAssignable)
	FOnHitDetected OnHitDetected;

protected:
	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	TObjectPtr<class UNiagaraSystem> ShockwaveSystem;

	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	TObjectPtr<class UNiagaraSystem> DistortionSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Debug")
	bool bShowDebugTrace = false;

	UPROPERTY()
	bool bIsHitChecking = false;

	UPROPERTY()
	float CurrentRadius;

	UPROPERTY()
	float CurrentDamage;

	UPROPERTY()
	FName CurrentSocketName;

	UPROPERTY()
	FVector PrevSocketLocation;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	FTimerHandle HitStopTimerHandle;
	bool bIsHitStopActive = false;
};
