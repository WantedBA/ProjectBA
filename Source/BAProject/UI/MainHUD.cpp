// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainHUD.h"
#include "UI/PlayerStatus.h"

void UMainHUD::UpdatePlayerHP(float Current, float Max)
{
	if (PlayerStatus)
	{
		PlayerStatus->UpdateHP(Current, Max);
	}
}
