// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "UI/System/PopupBase.h"
#include "DeathWIdget.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UDeathWidget : public UPopupBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
	// 마지막 체크포인트에서 다시 시작
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void RetryGame();

	// 타이틀 화면으로 돌아가기
	UFUNCTION(BlueprintCallable, Category = "BA|UI")
	void BackToTitle();

private:
	void FocusInitialDeathButton();
	bool TryNavigateDeathButton(const FVector2D& DirectionVector);
	bool SelectDeathButton(class UButton* Button);
	bool ActivateSelectedDeathButton();
	void FinishGamepadDeathButtonClick();
	void ClearGamepadSelectedDeathButton();

	TArray<class UButton*> GetNavigableDeathButtons() const;
	class UButton* ResolveCurrentDeathButton() const;
	class UButton* FindBestDeathButtonInDirection(class UButton* SourceButton, const FVector2D& DirectionVector) const;
	FVector2D GetDeathButtonCenterAbsolute(const class UButton* Button) const;
	void MoveMouseToDeathButton(const class UButton* Button);
	bool IsRightStickNavigationKey(const FKey& Key) const;
	FVector2D GetRightStickNavigationVector() const;
	bool HandleGamepadAcceptInput(const FKeyEvent& InKeyEvent);
	bool HandleGamepadBackInput(const FKeyEvent& InKeyEvent);

	TWeakObjectPtr<class UButton> GamepadSelectedButton;
	FVector2D RightStickNavigationInput = FVector2D::ZeroVector;
	FVector2D LastGamepadCursorAbsolute = FVector2D::ZeroVector;
	bool bRightStickNavigationReady = true;
	bool bGamepadNavigationActive = false;
	bool bGamepadClickPending = false;
	bool bBackToTitleRequested = false;
	FTimerHandle GamepadClickTimerHandle;
};
