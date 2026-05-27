// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainHUD.h"
#include "UI/PlayerStatus.h"
#include "UI/BossHPBar.h"
#include "UI/TooltipWidget.h"

void UMainHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// HUD가 생성되는 시점에 툴팁 생성
	if (TooltipClass)
	{
		TooltipInstance = CreateWidget<UTooltipWidget>(GetWorld(), TooltipClass);
	}
}

void UMainHUD::UpdatePlayerHP(float Current, float Max)
{
	if (PlayerStatus)
	{
		PlayerStatus->UpdateHP(Current, Max);
	}
}

void UMainHUD::InitBossStatus(FText Name, float Current, float Max)
{
	if (BossHPBar)
	{
		// 팝업이 안 되어 있다면 활성화
		if (BossHPBar->GetVisibility() != ESlateVisibility::Visible)
		{
			BossHPBar->SetAppearance(true);
		}

		// BossHPBar에 연출 실행
		BossHPBar->InitializeBossBar(Name, Max);
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

void UMainHUD::RequestShowTooltip(int32 InTid)
{
	if (TooltipInstance)
	{
		// 툴팁이 MainhHUD인 경우 최상단에 띄움
		if (!TooltipInstance->IsInViewport())
		{
			TooltipInstance->AddToViewport(1000);
		}

		// HUD가 들고 있는 Tooltip 위젯에게 전달
		TooltipInstance->RequestShowTooltip(InTid);
	}
}

void UMainHUD::HideTooltip()
{
	if (TooltipInstance)
	{
		// HUD가 들고 있는 Tooltip 위젯을 숨김
		TooltipInstance->HideTooltip();
	}
}

void UMainHUD::HideBossHPBar()
{
	if (BossHPBar)
	{
		BossHPBar->ResetBossBar();
	}
}
