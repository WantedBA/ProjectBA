// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "UI/System/PopupBase.h"
#include "TitleMenu.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UTitleMenu : public UPopupBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void ApplyTitleInputMode();
	void RestoreTitleInputMode();
	void FocusInitialTitleButton();
	bool TryNavigateTitleButton(const FVector2D& DirectionVector);
	bool SelectTitleButton(class UButton* Button);
	bool ActivateSelectedTitleButton();
	void FinishGamepadTitleButtonClick();
	void ClearGamepadSelectedTitleButton();

	TArray<class UButton*> GetNavigableTitleButtons() const;
	class UButton* ResolveCurrentTitleButton() const;
	class UButton* FindBestTitleButtonInDirection(class UButton* SourceButton, const FVector2D& DirectionVector) const;
	FVector2D GetTitleButtonCenterAbsolute(const class UButton* Button) const;
	void MoveMouseToTitleButton(const class UButton* Button);
	bool IsRightStickNavigationKey(const FKey& Key) const;
	FVector2D GetRightStickNavigationVector() const;

	TWeakObjectPtr<class UButton> GamepadSelectedButton;
	FVector2D RightStickNavigationInput = FVector2D::ZeroVector;
	FVector2D LastGamepadCursorAbsolute = FVector2D::ZeroVector;
	bool bRightStickNavigationReady = true;
	bool bGamepadNavigationActive = false;
	bool bGamepadClickPending = false;
	FTimerHandle GamepadClickTimerHandle;
};
