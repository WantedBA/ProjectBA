// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/PopupBase.h"
#include "SubSystemUI.h"

UPopupBase::UPopupBase(const FObjectInitializer& ObjectInitializer)
{
	bIsFocusable = true;

	SortOrder = 3;
}

void UPopupBase::ClosePopup()
{
	// 팝업 닫힐 때 스택에서 제거 요청
	if (USubSystemUI* UISubsystem = GetGameInstance()->GetSubsystem<USubSystemUI>())
	{
		UISubsystem->PopUI();
	}
}

FReply UPopupBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// "ESC" 또는 "3"번이 눌렸을 경우
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Three)
	{
		UE_LOG(LogTemp, Warning, TEXT("Key Pressed: %s"), *InKeyEvent.GetKey().ToString());

		// Subsystem 가져오기
		if (USubSystemUI* UIManager = GetGameInstance()->GetSubsystem<USubSystemUI>())
		{

			// 매니저에 뒤로가기 처리 요청
			UIManager->HandleBackAction();
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
