// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerStatus.h"
#include "StatusBar.h"

void UPlayerStatus::UpdateHP(float Current, float Max)
{
	if (HPBar)
	{
		HPBar->SetProgress(Current, Max);
	}
}

void UPlayerStatus::UpdateStamina(float Current, float Max)
{
	if (StaminaBar)
	{
		StaminaBar->SetProgress(Current, Max);
	}
}
