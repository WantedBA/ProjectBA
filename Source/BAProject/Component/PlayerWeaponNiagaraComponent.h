// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "PlayerWeaponNiagaraComponent.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UPlayerWeaponNiagaraComponent : public UNiagaraComponent
{
	GENERATED_BODY()
	
public:
	void ResetElement();
};
