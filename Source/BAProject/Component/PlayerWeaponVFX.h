// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "PlayerWeaponVFX.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UPlayerWeaponVFX : public USceneComponent
{
	GENERATED_BODY()

	
public:
	UPlayerWeaponVFX();
	
	// 모든 효과 끄기
	void ResetElement();
	
	void SetWeaponNiagaraAsset(UNiagaraSystem* NiagaraAsset);
	void SetTrailNiagaraAsset(UNiagaraSystem* NiagaraAsset);
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon VFX")
	UNiagaraComponent* WeaponNiagaraComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon VFX")
	UNiagaraComponent* TrailNiagaraComponent = nullptr;
};
