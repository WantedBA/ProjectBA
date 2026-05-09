// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/BAPopupBase.h"
#include "BASubSystemUI.h"

UBAPopupBase::UBAPopupBase(const FObjectInitializer& ObjectInitializer)
{
	SortOrder = 3;
}

void UBAPopupBase::ClosePopup()
{
	// 팝업 닫힐 때 스택에서 제거 요청
	if (UBASubSystemUI* UISubsystem = GetGameInstance()->GetSubsystem<UBASubSystemUI>())
	{
		UISubsystem->PopUI();
	}
}
