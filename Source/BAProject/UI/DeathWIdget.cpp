// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DeathWIdget.h"
#include "UI/System/SubSystemUI.h"
#include "Player/BAPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"

void UDeathWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UDeathWidget::RetryGame()
{
	// Respawn 실행
	if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(GetOwningPlayerPawn()))
	{
		Player->Respawn();
	}

	// 부활 시 사망 팝업 제거
	if (USubSystemUI* UISub = GetGameInstance()->GetSubsystem<USubSystemUI>())
	{
		UISub->PopUI();
	}
}

void UDeathWidget::BackToTitle()
{
	// 타이틀 이동 전 위젯 제거
	if (USubSystemUI* UISub = GetGameInstance()->GetSubsystem<USubSystemUI>())
	{
		UISub->ClearAllUI();
	}

	UGameplayStatics::OpenLevel(this, FName("TitleMap"));
}
