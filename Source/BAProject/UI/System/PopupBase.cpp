// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/PopupBase.h"
#include "SubSystemUI.h"

namespace
{
	FKey PopupGenericUSBControllerButton(const int32 ButtonNumber)
	{
		return FKey(FName(*FString::Printf(TEXT("GenericUSBController_Button%d"), ButtonNumber)));
	}

	bool IsBackKey(const FKey& Key)
	{
		return Key == EKeys::Escape
			|| Key == EKeys::Three
			|| Key == EKeys::Virtual_Back
			|| Key == EKeys::Gamepad_FaceButton_Right
			|| Key == PopupGenericUSBControllerButton(3);
	}
}

UPopupBase::UPopupBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	SetIsFocusable(true);

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
	if (IsBackKey(InKeyEvent.GetKey()))
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
