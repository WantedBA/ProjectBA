// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/SubSystemUI.h"
#include "StackElem.h"
#include "LayerBase.h"
#include "Blueprint/UserWidget.h"
#include "UI/MainHUD.h"

void USubSystemUI::PushUI(ULayerBase* InWidget)
{
	// 예외 체크
	if (!InWidget)
	{
		return;
	}

	if (UMainHUD* HUD = Cast<UMainHUD>(InWidget))
	{
		CachedMainHUD = HUD;
	}

	// 뷰포트에 위젯 띄움
	// 인터페이스의 GetSortOrder를 사용해 레이어 순서 결정
	InWidget->AddToViewport(InWidget->GetSortOrder());

	// 스택에 추가
	IStackElem* InterFacePtr = Cast<IStackElem>(InWidget);
	if (InterFacePtr)
	{
		UIStack.Push(InterFacePtr);
	}

	// 각 위젯이 OnPushed에서 로직 실행
	InWidget->OnPushed();

	RefreshInputMode();

	// 위젯이 키보드 입력을 받을 수 있도록 설정
	if (InWidget->GetStackType() == EStackElemType::Popup)
	{
		InWidget->SetKeyboardFocus();
	}


	UE_LOG(LogTemp, Log, TEXT("UI Pushed! 현재 스택 개수: %d"), UIStack.Num());
}

ULayerBase* USubSystemUI::PushUIByClass(TSubclassOf<ULayerBase> InWidgetClass)
{
	// 유효 체크
	if (!InWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("PushUIByClass: Invalid Widget Class"));
		return nullptr;
	}

	// 위젯 생성
	// GetWorld()를 통해 현재 월드에 위젯을 생성함
	ULayerBase* NewWidget = CreateWidget<ULayerBase>(GetWorld(), InWidgetClass);

	if (NewWidget)
	{
		// PushUI 함수 호출하여 스택에 쌓고 화면에 띄움
		PushUI(NewWidget);
		return NewWidget;
	}

	return nullptr;

}

void USubSystemUI::PopUI()
{
	// 스택이 비어있는지 확인
	if (UIStack.IsEmpty())
	{
		return;
	}

	// 가장 최근에 띄운 UI 끄기
	IStackElem* TopElem = UIStack.Pop();
	if (!TopElem)
	{
		return;
	}

	// LayerBase로 형변환 하여 위젯 기능 실행
	TopElem->OnPopped();

	// 애니메이션 재생
	if (ULayerBase* Widget = Cast<ULayerBase>(TopElem))
	{
		// 위젯 애니메이션 종료 시점을 수신하기 위해 델리게이트 바인딩
		Widget->OnCloseAnimationFinished.AddDynamic(this, &USubSystemUI::OnWidgetCloseAnimationFinished);

		// 위젯 내부의 닫기 로직(애니메이션 재생 등) 트리거
		Widget->StartCloseProcess();
	}

	RefreshInputMode();
	UE_LOG(LogTemp, Log, TEXT("UI Popped! 현재 스택 개수: %d"), UIStack.Num());

}

void USubSystemUI::OnWidgetCloseAnimationFinished(ULayerBase* Widget)
{
	if (Widget)
	{
		// 화면에서 지우고 메모리 정리
		Widget->RemoveFromParent();
		UE_LOG(LogTemp, Log, TEXT("[%s] 삭제 완료"), *Widget->GetName());
	}
}

void USubSystemUI::RefreshInputMode()
{
	// 현재 스택에 최상단 요소를 확인하여 팝업 유무 판별
	bool bHasPopup = false;

	if (!UIStack.IsEmpty())
	{
		IStackElem* TopElem = UIStack.Peek(); // 가장 위 요소 확인
		if (TopElem && TopElem->GetStackType() == EStackElemType::Popup)
		{
			bHasPopup = true;
		}
	}

	// 글로벌 블러 위젯 관리 로직
	if (bHasPopup)
	{
		// 블러 위젯 클래스가 설정되어 있고, 아직 생성되지 않았다면 생성
		if (!GlobalBlurWidget && BlurWidgetClass)
		{
			GlobalBlurWidget = CreateWidget<UUserWidget>(GetWorld(), BlurWidgetClass);
		}

		// 블러 위젯이 존재하고 현재 화면에 없다면 뷰포트에 추가
		// Order = HUD(1), Popup(3)
		if (GlobalBlurWidget && !GlobalBlurWidget->IsInViewport())
		{
			GlobalBlurWidget->AddToViewport(2);
		}
	}
	else
	{
		// 팝업이 한개도 없는 상태라면 화면에서 블러 위젯 제거
		if (GlobalBlurWidget && GlobalBlurWidget->IsInViewport())
		{
			GlobalBlurWidget->RemoveFromParent();
		}
	}

	// 입력 모드 및 마우스 커서 제어 로직
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		if (bHasPopup)
		{
			// 팝업이 있다면 UI 전용 입력 모드로 변경후 커서 활성화
			FInputModeUIOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
		else
		{
			// 팝업이 없다면 마우스 커서 비활성화, 게임 조작 재활성화
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
	}
}

bool USubSystemUI::HandleBackAction()
{
	// 스택이 비어있으면 통과
	if (UIStack.IsEmpty())
	{
		return false;
	}

	// 가장 위에 있는 위젯 확인
	IStackElem* TopElem = UIStack.Peek();

	// 가장 위에 있는 위젯이 Popup일 때만 닫기
	if (TopElem && TopElem->GetStackType() == EStackElemType::Popup)
	{
		PopUI();
		return true;
	}

	return false; // Popup이 없으면 아무것도 안함
}
