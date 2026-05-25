// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TitleMenu.h"
#include "GameFramework/PlayerController.h"

void UTitleMenu::NativeConstruct()
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
