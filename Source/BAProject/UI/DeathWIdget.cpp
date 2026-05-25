// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DeathWIdget.h"
#include "UI/System/SubSystemUI.h"
#include "Player/BAPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"

void UDeathWidget::NativeConstruct()
{
	Super::NativeConstruct();

	FTimerHandle FocusTimer;
	GetWorld()->GetTimerManager().SetTimer(FocusTimer, [this]()
		{
			APlayerController* PC = GetOwningPlayer();
			if (PC)
			{
				// 마우스 띄우기
				PC->bShowMouseCursor = true;

				// 입력 모드 고정
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(this->TakeWidget());
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

				PC->SetInputMode(InputMode);
				UE_LOG(LogTemp, Warning, TEXT(">>> Force Focus Set to: %s"), *GetName());
			}
		}, 0.1f, false);
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
