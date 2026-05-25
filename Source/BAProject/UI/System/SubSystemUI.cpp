// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/SubSystemUI.h"
#include "UI/MainHUD.h"
#include "UI/NotifyLayer.h"
#include "UI/System/UISettings.h"
#include "UI/DeathWidget.h"
#include "UI/System/PopupBase.h"
#include "StackElem.h"
#include "LayerBase.h"
#include "Blueprint/UserWidget.h"
#include "Component/StatComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include "Engine/Engine.h"
#include "Engine/World.h"


void USubSystemUI::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 월드 델리게이트에 함수 등록
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &USubSystemUI::HandleWorldInit);
}

void USubSystemUI::PushUI(ULayerBase* InWidget)
{
	// 예외 체크
	if (!InWidget)
	{
		return;
	}

	// Notify 저장
	if (UNotifyLayer* Notify = Cast<UNotifyLayer>(InWidget))
	{
		CachedNotifyLayer = Notify;
		UE_LOG(LogTemp, Warning, TEXT(">>> Success: CachedNotifyLayer is Set!"));
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

ULayerBase* USubSystemUI::PushUIByClass(TSubclassOf<ULayerBase> InWidgetClass, UWorld* InWorld)
{
	// 유효 체크
	if (!InWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("PushUIByClass: Invalid Widget Class"));
		return nullptr;
	}

	UWorld* TargetWorld = InWorld ? InWorld : GetWorld();

	// 위젯 생성
	// GetWorld()를 통해 현재 월드에 위젯을 생성함
	ULayerBase* NewWidget = CreateWidget<ULayerBase>(TargetWorld, InWidgetClass);

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

void USubSystemUI::HandleWorldInit(UWorld* World, const UWorld::InitializationValues IValues)
{
	CleanupUI();

	// 실제 플레이 시에만 실행
	if (World && World->IsGameWorld())
	{
		// 프로젝트 세팅에 등록된 설정값 가져오기
		const UUISettings* Settings = GetDefault<UUISettings>();

		if (Settings && Settings->DefaultNotifyLayerClass)
			{
				// 위젯 생성
				if (!CachedNotifyLayer || !CachedNotifyLayer->IsInViewport())
					{
						PushUIByClass(Settings->DefaultNotifyLayerClass, World);
					}
			}

		// 페이드 인 연출
		if (CachedNotifyLayer)
			{
				FTimerHandle TimerHandle;
				World->GetTimerManager().SetTimer(TimerHandle, [this, World]()
					{
						if (CachedNotifyLayer)
						{
							CachedNotifyLayer->PlayFadeEffect(true);
							CachedNotifyLayer->ShowSplashMessageByTid(10001);
						}

						if (APlayerController* PC = World->GetFirstPlayerController())
						{
							if (APawn* Pawn = PC->GetPawn())
							{
								if (UStatComponent* Stat = Pawn->FindComponentByClass<UStatComponent>())
								{
									// 플레이어 사망 시 델리게이트 연결
									Stat->OnDead.RemoveDynamic(this, &USubSystemUI::HandlePlayerDeath);
									Stat->OnDead.AddDynamic(this, &USubSystemUI::HandlePlayerDeath);

									UE_LOG(LogTemp, Warning, TEXT(">>> Success: Bound to Player Death Signal!"));
								}
							}
						}
					}, 0.1f, false);
			}
	}
}

void USubSystemUI::CleanupUI()
{
	ClearAllUI();

	// 캐시 포인터 초기화
	CachedMainHUD = nullptr;
	CachedNotifyLayer = nullptr;

	// 입력 모드 리셋
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		FInputModeGameOnly GameMode;
		PC->SetInputMode(GameMode);
	}
}

void USubSystemUI::HandlePlayerDeath()
{
	const UUISettings* Settings = GetDefault<UUISettings>();

	if (Settings && Settings->DeathWidgetClass)
	{
		TSubclassOf<ULayerBase> TargetLayerClass = Settings->DeathWidgetClass;

		// 팝업으로 사망 창 띄우기
		if (TargetLayerClass)
		{
			PushUIByClass(TargetLayerClass);
			UE_LOG(LogTemp, Warning, TEXT(">>> Player Died! Death Screen Pushed."));
		}

	}
}

void USubSystemUI::RefreshInputMode()
{
	// 현재 스택에 최상단 요소를 확인하여 팝업 유무 판별
	bool bHasPopup = false;
	ULayerBase* TopWidget = nullptr;


	if (!UIStack.IsEmpty())
	{
		IStackElem* TopElem = UIStack.Peek(); // 가장 위 요소 확인
		if (TopElem && TopElem->GetStackType() == EStackElemType::Popup)
		{
			bHasPopup = true;
			TopWidget = Cast<ULayerBase>(TopElem);
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
			if (TopWidget)
			{
				InputMode.SetWidgetToFocus(TopWidget->GetCachedWidget());
			}
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

void USubSystemUI::ClearAllUI()
{
	// 스택에 있는 모든 위젯을 화면에서 지우기
	while (!UIStack.IsEmpty())
	{
		IStackElem* Elem = UIStack.Pop();
		if (UUserWidget* Widget = Cast<UUserWidget>(Elem))
		{
			Widget->RemoveFromParent();
		}
	}

	// 캐싱된 변수 초기화
	CachedMainHUD = nullptr;
	CachedNotifyLayer = nullptr;
	GlobalBlurWidget = nullptr;

	UE_LOG(LogTemp, Log, TEXT(">>> SubSystemUI: All UI Cleared for Level Transition."));
}
