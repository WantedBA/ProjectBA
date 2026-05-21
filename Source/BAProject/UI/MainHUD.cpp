// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainHUD.h"
#include "UI/PlayerStatus.h"
#include "UI/BossHPBar.h"

void UMainHUD::UpdatePlayerHP(float Current, float Max)
{
	if (PlayerStatus)
	{
		PlayerStatus->UpdateHP(Current, Max);
	}
}

void UMainHUD::UpdateBossStatus(FText Name, float Current, float Max)
{
	if (BossHPBar)
	{
		// 보스 체력바가 숨겨져 있다면 즉시 표시
		if (BossHPBar->GetVisibility() != ESlateVisibility::Visible)
		{
			BossHPBar->SetAppearance(true);
		}

		// 이름, 체력 수치 갱신
		BossHPBar->SetBossName(Name);
		BossHPBar->UpdateHP(Current, Max);
	}
}
