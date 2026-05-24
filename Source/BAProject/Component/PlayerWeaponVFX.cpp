// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerWeaponVFX.h"

UPlayerWeaponVFX::UPlayerWeaponVFX()
{
	WeaponNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WeaponNiagaraComponent"));
	TrailNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailNiagaraComponent"));
}

void UPlayerWeaponVFX::ResetElement()
{
	WeaponNiagaraComponent->SetAsset(nullptr);
	WeaponNiagaraComponent->Deactivate();
	
	TrailNiagaraComponent->SetAsset(nullptr);
	TrailNiagaraComponent->Deactivate();
}

void UPlayerWeaponVFX::SetWeaponNiagaraAsset(UNiagaraSystem* NiagaraAsset)
{
	WeaponNiagaraComponent->SetAsset(NiagaraAsset);
	WeaponNiagaraComponent->Activate();
}

void UPlayerWeaponVFX::SetTrailNiagaraAsset(UNiagaraSystem* NiagaraAsset)
{
	TrailNiagaraComponent->SetAsset(NiagaraAsset);
	TrailNiagaraComponent->Activate();
}
