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

	if (!World || !World->IsGameWorld())
	{
		return;
	}

	// 기본 알림 레이어 생성
	if (const UUISettings* Settings = GetDefault<UUISettings>())
	{
		if (Settings->DefaultNotifyLayerClass && (!CachedNotifyLayer || !CachedNotifyLayer->IsInViewport()))
		{
			PushUIByClass(Settings->DefaultNotifyLayerClass, World);
		}
	}

	// 레벨 로드가 끝날 때까지 대기 후 한번에 처리
	FTimerHandle PostInitTimer;
	World->GetTimerManager().SetTimer(PostInitTimer, [this]()
		{
			UWorld* CurrentWorld = GetWorld();
			if (!CurrentWorld)
			{
				return;
			}

			// 입력 모드 동기화
			RefreshInputMode();

			// 페이드 인 및 시작 메시지 연출
			if (IsValid(CachedNotifyLayer) && CachedNotifyLayer->IsInViewport())
			{
				CachedNotifyLayer->PlayFadeEffect(true);
				CachedNotifyLayer->ShowSplashMessageByTid(10001); // 임시 텍스트값
			}

			// 플레이어 사망 신호 연결
			if (APlayerController* PC = CurrentWorld->GetFirstPlayerController())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					if (UStatComponent* Stat = Pawn->FindComponentByClass<UStatComponent>())
					{
						Stat->OnDead.RemoveDynamic(this, &USubSystemUI::HandlePlayerDeath);
						Stat->OnDead.AddDynamic(this, &USubSystemUI::HandlePlayerDeath);
						UE_LOG(LogTemp, Warning, TEXT(">>> Success: Bound to Player Death Signal!"));
					}
				}
			}
		}, 0.3f, false);
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
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	bool bHasPopup = false;
	ULayerBase* TopWidget = nullptr;

	if (!UIStack.IsEmpty())
	{
		IStackElem* TopElem = UIStack.Peek();
		if (TopElem && TopElem->GetStackType() == EStackElemType::Popup)
		{
			bHasPopup = true;
			TopWidget = Cast<ULayerBase>(TopElem);
		}
	}

	// 1. 글로벌 블러 위젯 관리 (기존 기능 유지)
	if (bHasPopup)
	{
		if (!GlobalBlurWidget && BlurWidgetClass)
		{
			GlobalBlurWidget = CreateWidget<UUserWidget>(GetWorld(), BlurWidgetClass);
		}
		if (GlobalBlurWidget && !GlobalBlurWidget->IsInViewport())
		{
			GlobalBlurWidget->AddToViewport(2);
		}
	}
	else
	{
		if (GlobalBlurWidget && GlobalBlurWidget->IsInViewport())
		{
			GlobalBlurWidget->RemoveFromParent();
		}
	}

	// 2. 입력 모드 및 포커스 정밀 진단
	if (bHasPopup && TopWidget)
	{

		FInputModeUIOnly InputMode;
		TSharedPtr<SWidget> SafeWidget = TopWidget->GetCachedWidget();

		if (SafeWidget.IsValid())
		{
			InputMode.SetWidgetToFocus(SafeWidget);
		}

		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;

		// 클릭 이벤트 최종 확인
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;

		// [강력 처방] 팝업이 뜨면 캐릭터 조작을 엔진 레벨에서 멈춥니다! ✨
		if (PC->GetPawn())
		{
			PC->GetPawn()->DisableInput(PC);
		}
	}
	else
	{
		FInputModeGameOnly GameMode;
		PC->SetInputMode(GameMode);
		PC->bShowMouseCursor = false;

		// [강력 처방] 팝업이 닫히면 조작을 다시 허용합니다. ✨
		if (PC->GetPawn())
		{
			PC->GetPawn()->EnableInput(PC);
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
	if (GlobalBlurWidget)
	{
		GlobalBlurWidget->RemoveFromParent();
		GlobalBlurWidget = nullptr;
	}
}
