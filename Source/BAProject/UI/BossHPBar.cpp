// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/BossHPBar.h"
#include "Components/TextBlock.h"
#include "UI/StatusBar.h"

void UBossHPBar::SetBossName(FText NewName)
{
	if (BossNameText)
	{
		BossNameText->SetText(NewName);
	}
}

void UBossHPBar::UpdateHP(float CurrentHP, float MaxHP)
{
	if (HPBar)
	{
		// StatusBar의 SetProgress를 호출하여 애니메이션 실행
		HPBar->SetProgress(CurrentHP, MaxHP);
	}
}

void UBossHPBar::SetAppearance(bool bVisible)
{
	// 인자(bVisible)에 따라 보스 체력바의 화면 표시 여부를 동적으로 전환
	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}