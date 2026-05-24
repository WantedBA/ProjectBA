// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerWeaponNiagaraComponent.h"

void UPlayerWeaponNiagaraComponent::ResetElement()
{
	SetAsset(nullptr);
	Deactivate();
}
