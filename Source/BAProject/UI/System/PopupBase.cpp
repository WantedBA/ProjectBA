// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/PopupBase.h"
#include "SubSystemUI.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

	bool IsAcceptKey(const FKey& Key)
	{
		return Key == EKeys::Enter
			|| Key == EKeys::Virtual_Accept
			|| Key == EKeys::Gamepad_FaceButton_Bottom
			|| Key == PopupGenericUSBControllerButton(1)
			|| Key == PopupGenericUSBControllerButton(2);
	}

	void CollectFocusableButtons(UWidgetTree* InWidgetTree, UUserWidget* OwnerWidget, TArray<UButton*>& OutButtons)
	{
		if (!InWidgetTree)
		{
			return;
		}

		TArray<UWidget*> Widgets;
		InWidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (!Widget || Widget == OwnerWidget)
			{
				continue;
			}

			if (UButton* Button = Cast<UButton>(Widget))
			{
				if (Button->GetIsEnabled() && Button->GetVisibility() != ESlateVisibility::Collapsed)
				{
					OutButtons.Add(Button);
				}
				continue;
			}

			if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
			{
				CollectFocusableButtons(UserWidget->WidgetTree, OwnerWidget, OutButtons);
			}
		}
	}
}

UPopupBase::UPopupBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	SetIsFocusable(true);

	SortOrder = 3;
}

void UPopupBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UPopupBase::FocusFirstGamepadNavigableWidget));
	}
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
	if (IsAcceptKey(InKeyEvent.GetKey()) && HandleAcceptKey())
	{
		return FReply::Handled();
	}

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

bool UPopupBase::HandleAcceptKey()
{
	UButton* ButtonToClick = FindFocusedButton();
	if (!ButtonToClick)
	{
		FocusFirstGamepadNavigableWidget();
		ButtonToClick = FindFocusedButton();
	}

	if (!ButtonToClick)
	{
		return false;
	}

	ActivateFocusedButton(ButtonToClick);
	return true;
}

void UPopupBase::ActivateFocusedButton(UButton* ButtonToClick) const
{
	if (!ButtonToClick)
	{
		return;
	}

	ButtonToClick->OnPressed.Broadcast();
	ButtonToClick->OnReleased.Broadcast();
	ButtonToClick->OnClicked.Broadcast();
}

UButton* UPopupBase::FindFocusedButton() const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	TArray<UButton*> Buttons;
	CollectFocusableButtons(WidgetTree, const_cast<UPopupBase*>(this), Buttons);
	for (UButton* Button : Buttons)
	{
		if (Button && (Button->HasKeyboardFocus() || Button->HasAnyUserFocus()))
		{
			return Button;
		}
	}

	return Buttons.IsEmpty() ? nullptr : Buttons[0];
}

void UPopupBase::FocusFirstGamepadNavigableWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	TArray<UButton*> Buttons;
	CollectFocusableButtons(WidgetTree, this, Buttons);
	if (Buttons.IsEmpty())
	{
		return;
	}

	UButton* FirstButton = Buttons[0];
	if (!FirstButton)
	{
		return;
	}

	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		FirstButton->SetUserFocus(OwningPlayer);
	}
	FirstButton->SetKeyboardFocus();
}
