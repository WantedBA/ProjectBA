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
	
	virtual void OnRegister() override;
	
	// 모든 효과 끄기
	void ResetElement();
	
	// 무기 나이아가라 설정
	void SetWeaponNiagaraAsset(UNiagaraSystem* NiagaraAsset);
	
	// 기본값은 콜리전이 켜졌을 때만 켜질 수 있게 deactivate 상태로 둡니다
	void SetTrailNiagaraAsset(UNiagaraSystem* NiagaraAsset, bool bActivate = false);
	
	void ActivateTrailNiagara();
	void DeactivateTrailNiagara();
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon VFX")
	UNiagaraComponent* WeaponNiagaraComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon VFX")
	UNiagaraComponent* TrailNiagaraComponent = nullptr;
	
	bool bIsTrailNiagaraAlwaysActivated = false;
};
