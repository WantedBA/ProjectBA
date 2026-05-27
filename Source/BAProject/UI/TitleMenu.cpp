// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TitleMenu.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace
{
	constexpr float TitleRightStickNavigationThreshold = 0.85f;
	constexpr float TitleRightStickNavigationResetThreshold = 0.35f;
	constexpr float TitleNavigationMinDirectionDot = 0.35f;
	constexpr float TitleNavigationSmallDistance = 1.0f;
	constexpr float TitleGamepadMouseMoveTolerance = 3.0f;

	FKey TitleGenericUSBControllerButton(const int32 ButtonNumber)
	{
		return FKey(FName(*FString::Printf(TEXT("GenericUSBController_Button%d"), ButtonNumber)));
	}

	bool IsTitleAcceptKey(const FKey& Key)
	{
		return Key == EKeys::Enter
			|| Key == EKeys::Virtual_Accept
			|| Key == EKeys::Gamepad_FaceButton_Bottom
			|| Key == TitleGenericUSBControllerButton(1)
			|| Key == TitleGenericUSBControllerButton(2);
	}

	bool IsTitleButtonNavigable(const UButton* Button)
	{
		if (!Button || !Button->GetIsEnabled())
		{
			return false;
		}

		const ESlateVisibility Visibility = Button->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed
			&& Visibility != ESlateVisibility::Hidden;
	}

	void CollectTitleButtons(UWidgetTree* InWidgetTree, TArray<UButton*>& OutButtons)
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
				if (IsTitleButtonNavigable(Button))
				{
					OutButtons.AddUnique(Button);
				}
				continue;
			}

			if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
			{
				CollectTitleButtons(UserWidget->WidgetTree, OutButtons);
			}
		}
	}

	FVector2D NormalizeTitleNavigationVector(const FVector2D& DirectionVector)
	{
		return DirectionVector.GetSafeNormal();
	}
}

void UTitleMenu::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyTitleInputMode();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UTitleMenu::FocusInitialTitleButton));
	}
	else
	{
		FocusInitialTitleButton();
	}
}

void UTitleMenu::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GamepadClickTimerHandle);
	}

	ClearGamepadSelectedTitleButton();
	RestoreTitleInputMode();
	Super::NativeDestruct();
}

FReply UTitleMenu::NativeOnAnalogValueChanged(
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
	if (StrongestInput < TitleRightStickNavigationResetThreshold)
	{
		bRightStickNavigationReady = true;
		return FReply::Handled();
	}

	if (!bRightStickNavigationReady || StrongestInput < TitleRightStickNavigationThreshold)
	{
		return FReply::Handled();
	}

	bRightStickNavigationReady = false;
	TryNavigateTitleButton(GetRightStickNavigationVector());
	return FReply::Handled();
}

FReply UTitleMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsTitleAcceptKey(InKeyEvent.GetKey()) && ActivateSelectedTitleButton())
	{
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UTitleMenu::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsTitleAcceptKey(InKeyEvent.GetKey()) && ActivateSelectedTitleButton())
	{
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UTitleMenu::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bGamepadNavigationActive
		&& FVector2D::Distance(InMouseEvent.GetScreenSpacePosition(), LastGamepadCursorAbsolute) > TitleGamepadMouseMoveTolerance)
	{
		bGamepadNavigationActive = false;
		ClearGamepadSelectedTitleButton();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UTitleMenu::ApplyTitleInputMode()
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return;
	}

	FInputModeUIOnly InputMode;
	TSharedPtr<SWidget> SafeWidget = GetCachedWidget();
	if (SafeWidget.IsValid())
	{
		InputMode.SetWidgetToFocus(SafeWidget);
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	OwningPlayer->SetInputMode(InputMode);
	OwningPlayer->bShowMouseCursor = true;
	OwningPlayer->bEnableClickEvents = true;
	OwningPlayer->bEnableMouseOverEvents = true;
}

void UTitleMenu::RestoreTitleInputMode()
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return;
	}

	FInputModeGameOnly InputMode;
	OwningPlayer->SetInputMode(InputMode);
	OwningPlayer->bShowMouseCursor = false;
	OwningPlayer->bEnableClickEvents = false;
	OwningPlayer->bEnableMouseOverEvents = false;
}

void UTitleMenu::FocusInitialTitleButton()
{
	ApplyTitleInputMode();

	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		SetUserFocus(OwningPlayer);
	}
	SetKeyboardFocus();
}

bool UTitleMenu::TryNavigateTitleButton(const FVector2D& DirectionVector)
{
	UButton* CurrentButton = ResolveCurrentTitleButton();
	if (!CurrentButton)
	{
		return false;
	}

	if (UButton* TargetButton = FindBestTitleButtonInDirection(CurrentButton, DirectionVector))
	{
		return SelectTitleButton(TargetButton);
	}

	return SelectTitleButton(CurrentButton);
}

bool UTitleMenu::SelectTitleButton(UButton* Button)
{
	if (!IsTitleButtonNavigable(Button))
	{
		return false;
	}

	if (GamepadSelectedButton.Get() != Button)
	{
		ClearGamepadSelectedTitleButton();
		GamepadSelectedButton = Button;
		Button->OnHovered.Broadcast();
	}

	bGamepadNavigationActive = true;
	MoveMouseToTitleButton(Button);

	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		SetUserFocus(OwningPlayer);
	}
	SetKeyboardFocus();

	return true;
}

bool UTitleMenu::ActivateSelectedTitleButton()
{
	if (!GamepadSelectedButton.IsValid())
	{
		if (!SelectTitleButton(ResolveCurrentTitleButton()))
		{
			return false;
		}
	}

	UButton* Button = GamepadSelectedButton.Get();
	if (!IsTitleButtonNavigable(Button) || bGamepadClickPending)
	{
		return false;
	}

	MoveMouseToTitleButton(Button);
	bGamepadClickPending = true;
	Button->OnPressed.Broadcast();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GamepadClickTimerHandle,
			this,
			&UTitleMenu::FinishGamepadTitleButtonClick,
			0.05f,
			false);
	}
	else
	{
		FinishGamepadTitleButtonClick();
	}

	return true;
}

void UTitleMenu::FinishGamepadTitleButtonClick()
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

void UTitleMenu::ClearGamepadSelectedTitleButton()
{
	if (UButton* PreviousButton = GamepadSelectedButton.Get())
	{
		PreviousButton->OnUnhovered.Broadcast();
	}

	GamepadSelectedButton.Reset();
}

TArray<UButton*> UTitleMenu::GetNavigableTitleButtons() const
{
	TArray<UButton*> Buttons;
	CollectTitleButtons(WidgetTree, Buttons);
	return Buttons;
}

UButton* UTitleMenu::ResolveCurrentTitleButton() const
{
	const TArray<UButton*> Buttons = GetNavigableTitleButtons();
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
					const_cast<UTitleMenu*>(this),
					Button->GetCachedGeometry(),
					FVector2D::ZeroVector,
					TopLeftPixel,
					TopLeftViewport);
				USlateBlueprintLibrary::LocalToViewport(
					const_cast<UTitleMenu*>(this),
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

UButton* UTitleMenu::FindBestTitleButtonInDirection(UButton* SourceButton, const FVector2D& DirectionVector) const
{
	if (!SourceButton)
	{
		return nullptr;
	}

	const FVector2D NormalizedDirection = NormalizeTitleNavigationVector(DirectionVector);
	if (NormalizedDirection.IsNearlyZero())
	{
		return nullptr;
	}

	const FVector2D SourcePosition = GetTitleButtonCenterAbsolute(SourceButton);
	UButton* BestButton = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (UButton* CandidateButton : GetNavigableTitleButtons())
	{
		if (!CandidateButton || CandidateButton == SourceButton)
		{
			continue;
		}

		const FVector2D Delta = GetTitleButtonCenterAbsolute(CandidateButton) - SourcePosition;
		const float DistanceSquared = Delta.SizeSquared();
		if (DistanceSquared <= TitleNavigationSmallDistance)
		{
			continue;
		}

		const float DirectionDot = FVector2D::DotProduct(Delta.GetSafeNormal(), NormalizedDirection);
		if (DirectionDot < TitleNavigationMinDirectionDot)
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

FVector2D UTitleMenu::GetTitleButtonCenterAbsolute(const UButton* Button) const
{
	if (!Button)
	{
		return FVector2D::ZeroVector;
	}

	const FGeometry& Geometry = Button->GetCachedGeometry();
	return Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f);
}

void UTitleMenu::MoveMouseToTitleButton(const UButton* Button)
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer || !Button)
	{
		return;
	}

	LastGamepadCursorAbsolute = GetTitleButtonCenterAbsolute(Button);
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(this, LastGamepadCursorAbsolute, PixelPosition, ViewportPosition);
	OwningPlayer->SetMouseLocation(FMath::RoundToInt(PixelPosition.X), FMath::RoundToInt(PixelPosition.Y));
}

bool UTitleMenu::IsRightStickNavigationKey(const FKey& Key) const
{
	return Key == EKeys::Gamepad_RightX || Key == EKeys::Gamepad_RightY;
}

FVector2D UTitleMenu::GetRightStickNavigationVector() const
{
	return FVector2D(RightStickNavigationInput.X, -RightStickNavigationInput.Y).GetSafeNormal();
}
