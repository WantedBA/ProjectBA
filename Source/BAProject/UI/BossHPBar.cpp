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

void UBossHPBar::InitializeBossBar(FText Name, float MaxHP)
{
	if (HPBar)
	{
		// 체력을 0 으로 리셋
		HPBar->SetProgressImmediate(0.0f);

		// 보스 이름 설정
		SetBossName(Name);

		// 외형 보이기
		SetAppearance(true);

		// 체력을 100%로 올리기
		HPBar->SetProgress(MaxHP, MaxHP);
	}
}

void UBossHPBar::ResetBossBar()
{
	// 이름 텍스트 비우기
	if (BossNameText)
	{
		BossNameText->SetText(FText::GetEmpty());
	}

	// 게이지 초기화
	if (HPBar)
	{
		HPBar->SetProgress(0.f, 100.f);
	}

	// 화면에서 숨기기
	SetAppearance(false);
}
