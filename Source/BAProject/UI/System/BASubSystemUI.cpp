// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/BASubSystemUI.h"
#include "BAStackElem.h"
#include "BALayerBase.h"
#include "Blueprint/UserWidget.h"

void UBASubSystemUI::PushUI(UBALayerBase* InWidget)
{
	// 예외 체크
	if (!InWidget)
	{
		return;
	}

	// 뷰포트에 위젯 띄움
	// 인터페이스의 GetSortOrder를 사용해 레이어 순서 결정
	InWidget->AddToViewport(InWidget->GetSortOrder());

	// 스택에 추가
	IBAStackElem* InterFacePtr = Cast<IBAStackElem>(InWidget);
	if (InterFacePtr)
	{
		UIStack.Push(InterFacePtr);
	}

	// 각 위젯이 OnPushed에서 로직 실행
	InWidget->OnPushed();

	UE_LOG(LogTemp, Log, TEXT("UI Pushed! 현재 스택 개수: %d"), UIStack.Num());
}

void UBASubSystemUI::PopUI()
{
	// 스택이 비어있는지 확인
	if (UIStack.IsEmpty())
	{
		return;
	}

	// 가장 최근에 띄운 UI 끄기
	IBAStackElem* TopElem = UIStack.Pop();
	if (!TopElem)
	{
		return;
	}

	// BALayerBase로 형변환 하여 위젯 기능 실행
	TopElem->OnPopped();

	// 화면에서 제거
	if (UUserWidget* Widget = Cast<UUserWidget>(TopElem))
	{
		Widget->RemoveFromParent(); // 메모리 관리
	}

	UE_LOG(LogTemp, Log, TEXT("UI Popped! 현재 스택 개수: %d"), UIStack.Num());

}
