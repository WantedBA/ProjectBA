// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerWeaponVFX.h"

UPlayerWeaponVFX::UPlayerWeaponVFX()
{
}

void UPlayerWeaponVFX::OnRegister()
{
	Super::OnRegister();
	
	if (IsTemplate())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!WeaponNiagaraComponent)
	{
		WeaponNiagaraComponent = NewObject<UNiagaraComponent>(Owner, TEXT("WeaponNiagaraComponent"));
		WeaponNiagaraComponent->SetAutoActivate(false);
		WeaponNiagaraComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		WeaponNiagaraComponent->RegisterComponent();
	}

	if (!TrailNiagaraComponent)
	{
		TrailNiagaraComponent = NewObject<UNiagaraComponent>(Owner, TEXT("TrailNiagaraComponent"));
		TrailNiagaraComponent->SetAutoActivate(false);
		TrailNiagaraComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		TrailNiagaraComponent->RegisterComponent();
	}
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

void UPlayerWeaponVFX::SetTrailNiagaraAsset(UNiagaraSystem* NiagaraAsset, bool bActivate)
{
	TrailNiagaraComponent->SetAsset(NiagaraAsset);
	if (bActivate)
	{
		TrailNiagaraComponent->Activate();
		bIsTrailNiagaraAlwaysActivated = true;
	}
	else
	{
		TrailNiagaraComponent->Deactivate();
		bIsTrailNiagaraAlwaysActivated = false;
	}
}

void UPlayerWeaponVFX::ActivateTrailNiagara()
{
	TrailNiagaraComponent->Activate(true);
}

void UPlayerWeaponVFX::DeactivateTrailNiagara()
{
	TrailNiagaraComponent->Deactivate();
}
