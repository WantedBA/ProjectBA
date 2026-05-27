// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DeathWIdget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UI/System/SubSystemUI.h"
#include "UI/MainHUD.h"
#include "UI/NotifyLayer.h"
#include "Player/BAPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float DeathRightStickNavigationThreshold = 0.85f;
	constexpr float DeathRightStickNavigationResetThreshold = 0.35f;
	constexpr float DeathNavigationMinDirectionDot = 0.35f;
	constexpr float DeathNavigationSmallDistance = 1.0f;
	constexpr float DeathGamepadMouseMoveTolerance = 3.0f;

	FKey DeathGenericUSBControllerButton(const int32 ButtonNumber)
	{
		return FKey(FName(*FString::Printf(TEXT("GenericUSBController_Button%d"), ButtonNumber)));
	}

	bool IsDeathAcceptKey(const FKey& Key)
	{
		return Key == EKeys::Enter
			|| Key == EKeys::Virtual_Accept
			|| Key == EKeys::Gamepad_FaceButton_Bottom
			|| Key == DeathGenericUSBControllerButton(1)
			|| Key == DeathGenericUSBControllerButton(2);
	}

	bool IsDeathBackKey(const FKey& Key)
	{
		return Key == EKeys::Escape
			|| Key == EKeys::Virtual_Back
			|| Key == EKeys::Gamepad_FaceButton_Right
			|| Key == DeathGenericUSBControllerButton(3);
	}

	bool IsDeathButtonNavigable(const UButton* Button)
	{
		if (!Button || !Button->GetIsEnabled())
		{
			return false;
		}

		const ESlateVisibility Visibility = Button->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed
			&& Visibility != ESlateVisibility::Hidden;
	}

	void CollectDeathButtons(UWidgetTree* InWidgetTree, TArray<UButton*>& OutButtons)
	{
		if (!InWidgetTree)
		{
			return;
		}

		TArray<UWidget*> Widgets;
		InWidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (!Widget)
			{
				continue;
			}

			if (UButton* Button = Cast<UButton>(Widget))
			{
				if (IsDeathButtonNavigable(Button))
				{
					OutButtons.AddUnique(Button);
				}
				continue;
			}

			if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
			{
				CollectDeathButtons(UserWidget->WidgetTree, OutButtons);
			}
		}
	}

	FVector2D NormalizeDeathNavigationVector(const FVector2D& DirectionVector)
	{
		return DirectionVector.GetSafeNormal();
	}
}

void UDeathWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UDeathWidget::FocusInitialDeathButton));
	}
	else
	{
		FocusInitialDeathButton();
	}
}

void UDeathWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GamepadClickTimerHandle);
	}

	ClearGamepadSelectedDeathButton();
	Super::NativeDestruct();
}

FReply UDeathWidget::NativeOnAnalogValueChanged(
	const FGeometry& InGeometry,
	const FAnalogInputEvent& InAnalogEvent)
{
	if (!IsRightStickNavigationKey(InAnalogEvent.GetKey()))
	{
		return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
	}

	if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightX)
	{
		RightStickNavigationInput.X = InAnalogEvent.GetAnalogValue();
	}
	else if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightY)
	{
		RightStickNavigationInput.Y = InAnalogEvent.GetAnalogValue();
	}

	const float StrongestInput = FMath::Max(FMath::Abs(RightStickNavigationInput.X), FMath::Abs(RightStickNavigationInput.Y));
	if (StrongestInput < DeathRightStickNavigationResetThreshold)
	{
		bRightStickNavigationReady = true;
		return FReply::Handled();
	}

	if (!bRightStickNavigationReady || StrongestInput < DeathRightStickNavigationThreshold)
	{
		return FReply::Handled();
	}

	bRightStickNavigationReady = false;
	TryNavigateDeathButton(GetRightStickNavigationVector());
	return FReply::Handled();
}

FReply UDeathWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (HandleGamepadBackInput(InKeyEvent) || HandleGamepadAcceptInput(InKeyEvent))
	{
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UDeathWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (HandleGamepadBackInput(InKeyEvent) || HandleGamepadAcceptInput(InKeyEvent))
	{
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UDeathWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bGamepadNavigationActive
		&& FVector2D::Distance(InMouseEvent.GetScreenSpacePosition(), LastGamepadCursorAbsolute) > DeathGamepadMouseMoveTolerance)
	{
		bGamepadNavigationActive = false;
		ClearGamepadSelectedDeathButton();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UDeathWidget::RetryGame()
{
	// Respawn 실행
	if (ABAPlayerCharacter* Player = Cast<ABAPlayerCharacter>(GetOwningPlayerPawn()))
	{
		Player->Respawn();
	}

	if (USubSystemUI* UISub = GetGameInstance()->GetSubsystem<USubSystemUI>())
	{
		if (UNotifyLayer* Notify = UISub->GetNotifyLayer())
		{
			Notify->PlayFadeEffect(true);
		}
	}

	if (USubSystemUI* UISub = GetGameInstance()->GetSubsystem<USubSystemUI>())
	{
		if (UMainHUD* MainHUD = UISub->GetMainHUD())
		{
			MainHUD->HideBossHPBar();
		}

		// 부활 시 사망 팝업 제거
		UISub->PopUI();
	}
}

void UDeathWidget::BackToTitle()
{
	if (bBackToTitleRequested)
	{
		return;
	}

	bBackToTitleRequested = true;

	// 타이틀 이동 전 위젯 제거
	if (USubSystemUI* UISub = GetGameInstance()->GetSubsystem<USubSystemUI>())
	{
		UISub->ClearAllUI();
	}

	UGameplayStatics::OpenLevel(this, FName("TitleMap"));
}

void UDeathWidget::FocusInitialDeathButton()
{
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		SetUserFocus(OwningPlayer);
	}
	SetKeyboardFocus();
	SelectDeathButton(ResolveCurrentDeathButton());
}

bool UDeathWidget::TryNavigateDeathButton(const FVector2D& DirectionVector)
{
	UButton* CurrentButton = ResolveCurrentDeathButton();
	if (!CurrentButton)
	{
		return false;
	}

	if (UButton* TargetButton = FindBestDeathButtonInDirection(CurrentButton, DirectionVector))
	{
		return SelectDeathButton(TargetButton);
	}

	return SelectDeathButton(CurrentButton);
}

bool UDeathWidget::SelectDeathButton(UButton* Button)
{
	if (!IsDeathButtonNavigable(Button))
	{
		return false;
	}

	if (GamepadSelectedButton.Get() != Button)
	{
		ClearGamepadSelectedDeathButton();
		GamepadSelectedButton = Button;
		Button->OnHovered.Broadcast();
	}

	bGamepadNavigationActive = true;
	MoveMouseToDeathButton(Button);

	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		SetUserFocus(OwningPlayer);
	}
	SetKeyboardFocus();

	return true;
}

bool UDeathWidget::ActivateSelectedDeathButton()
{
	if (!GamepadSelectedButton.IsValid())
	{
		if (!SelectDeathButton(ResolveCurrentDeathButton()))
		{
			return false;
		}
	}

	UButton* Button = GamepadSelectedButton.Get();
	if (!IsDeathButtonNavigable(Button) || bGamepadClickPending)
	{
		return false;
	}

	MoveMouseToDeathButton(Button);
	bGamepadClickPending = true;
	Button->OnPressed.Broadcast();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GamepadClickTimerHandle,
			this,
			&UDeathWidget::FinishGamepadDeathButtonClick,
			0.05f,
			false);
	}
	else
	{
		FinishGamepadDeathButtonClick();
	}

	return true;
}

void UDeathWidget::FinishGamepadDeathButtonClick()
{
	UButton* Button = GamepadSelectedButton.Get();
	if (!Button)
	{
		bGamepadClickPending = false;
		return;
	}

	Button->OnReleased.Broadcast();
	Button->OnClicked.Broadcast();
	bGamepadClickPending = false;
}

void UDeathWidget::ClearGamepadSelectedDeathButton()
{
	if (UButton* PreviousButton = GamepadSelectedButton.Get())
	{
		PreviousButton->OnUnhovered.Broadcast();
	}

	GamepadSelectedButton.Reset();
}

TArray<UButton*> UDeathWidget::GetNavigableDeathButtons() const
{
	TArray<UButton*> Buttons;
	CollectDeathButtons(WidgetTree, Buttons);
	return Buttons;
}

UButton* UDeathWidget::ResolveCurrentDeathButton() const
{
	const TArray<UButton*> Buttons = GetNavigableDeathButtons();
	if (Buttons.Contains(GamepadSelectedButton.Get()))
	{
		return GamepadSelectedButton.Get();
	}

	for (UButton* Button : Buttons)
	{
		if (Button && (Button->HasKeyboardFocus() || Button->HasAnyUserFocus()))
		{
			return Button;
		}
	}

	const APlayerController* OwningPlayer = GetOwningPlayer();
	if (OwningPlayer)
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		if (OwningPlayer->GetMousePosition(MouseX, MouseY))
		{
			const FVector2D MousePosition(MouseX, MouseY);
			for (UButton* Button : Buttons)
			{
				if (!Button)
				{
					continue;
				}

				FVector2D TopLeftPixel;
				FVector2D TopLeftViewport;
				FVector2D BottomRightPixel;
				FVector2D BottomRightViewport;
				USlateBlueprintLibrary::LocalToViewport(
					const_cast<UDeathWidget*>(this),
					Button->GetCachedGeometry(),
					FVector2D::ZeroVector,
					TopLeftPixel,
					TopLeftViewport);
				USlateBlueprintLibrary::LocalToViewport(
					const_cast<UDeathWidget*>(this),
					Button->GetCachedGeometry(),
					Button->GetCachedGeometry().GetLocalSize(),
					BottomRightPixel,
					BottomRightViewport);

				if (MousePosition.X >= TopLeftPixel.X
					&& MousePosition.X <= BottomRightPixel.X
					&& MousePosition.Y >= TopLeftPixel.Y
					&& MousePosition.Y <= BottomRightPixel.Y)
				{
					return Button;
				}
			}
		}
	}

	return Buttons.IsEmpty() ? nullptr : Buttons[0];
}

UButton* UDeathWidget::FindBestDeathButtonInDirection(UButton* SourceButton, const FVector2D& DirectionVector) const
{
	if (!SourceButton)
	{
		return nullptr;
	}

	const FVector2D NormalizedDirection = NormalizeDeathNavigationVector(DirectionVector);
	if (NormalizedDirection.IsNearlyZero())
	{
		return nullptr;
	}

	const FVector2D SourcePosition = GetDeathButtonCenterAbsolute(SourceButton);
	UButton* BestButton = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (UButton* CandidateButton : GetNavigableDeathButtons())
	{
		if (!CandidateButton || CandidateButton == SourceButton)
		{
			continue;
		}

		const FVector2D Delta = GetDeathButtonCenterAbsolute(CandidateButton) - SourcePosition;
		const float DistanceSquared = Delta.SizeSquared();
		if (DistanceSquared <= DeathNavigationSmallDistance)
		{
			continue;
		}

		const float DirectionDot = FVector2D::DotProduct(Delta.GetSafeNormal(), NormalizedDirection);
		if (DirectionDot < DeathNavigationMinDirectionDot)
		{
			continue;
		}

		const float Score = DistanceSquared * (2.0f - DirectionDot);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestButton = CandidateButton;
		}
	}

	return BestButton;
}

FVector2D UDeathWidget::GetDeathButtonCenterAbsolute(const UButton* Button) const
{
	if (!Button)
	{
		return FVector2D::ZeroVector;
	}

	const FGeometry& Geometry = Button->GetCachedGeometry();
	return Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f);
}

void UDeathWidget::MoveMouseToDeathButton(const UButton* Button)
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer || !Button)
	{
		return;
	}

	LastGamepadCursorAbsolute = GetDeathButtonCenterAbsolute(Button);
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(this, LastGamepadCursorAbsolute, PixelPosition, ViewportPosition);
	OwningPlayer->SetMouseLocation(FMath::RoundToInt(PixelPosition.X), FMath::RoundToInt(PixelPosition.Y));
}

bool UDeathWidget::IsRightStickNavigationKey(const FKey& Key) const
{
	return Key == EKeys::Gamepad_RightX || Key == EKeys::Gamepad_RightY;
}

FVector2D UDeathWidget::GetRightStickNavigationVector() const
{
	return FVector2D(RightStickNavigationInput.X, -RightStickNavigationInput.Y).GetSafeNormal();
}

bool UDeathWidget::HandleGamepadAcceptInput(const FKeyEvent& InKeyEvent)
{
	if (!IsDeathAcceptKey(InKeyEvent.GetKey()))
	{
		return false;
	}

	if (!InKeyEvent.IsRepeat())
	{
		ActivateSelectedDeathButton();
	}
	return true;
}

bool UDeathWidget::HandleGamepadBackInput(const FKeyEvent& InKeyEvent)
{
	if (!IsDeathBackKey(InKeyEvent.GetKey()))
	{
		return false;
	}

	if (!InKeyEvent.IsRepeat())
	{
		BackToTitle();
	}
	return true;
}
