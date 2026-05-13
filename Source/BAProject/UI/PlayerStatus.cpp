// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerStatus.h"
#include "StatusBar.h"
#include "Instance/UserDataSubsystem.h"

void UPlayerStatus::NativeConstruct()
{
	Super::NativeConstruct();

	// 전역 데이터 서브시스템 인스턴스 가져오기
	UUserDataSubsystem* UserData = GetGameInstance()->GetSubsystem<UUserDataSubsystem>();

	if (UserData)
	{
		// [체력] 서브시스템의 HP 변경을 UpdateHP 함수에 등록
		UserData->OnHpChanged.AddDynamic(this, &UPlayerStatus::UpdateHP);

		// [스테미너] 서브시스템의 Stamina 변경을 UpdateStamina 함수에 등록
		UserData->OnStaminaChanged.AddDynamic(this, &UPlayerStatus::UpdateStamina);

	}
}

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
