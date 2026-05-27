// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Instance/SkillTreeTypes.h"
#include "TimerManager.h"
#include "SkillNodeWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillNodeClickedDelegate, int32, SkillId);

/**
 * 
 */
UCLASS()
class BAPROJECT_API USkillNodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Getter
	[[nodiscard]] ESkillNodeState GetSkillNodeState() const
	{
		return SkillNodeState;
	}

	[[nodiscard]] int32 GetSkillId() const
	{
		return SkillId;
	}

	[[nodiscard]] class UButton* GetSkillNodeButton() const
	{
		return SkillNodeButton;
	}

	void SetGamepadHoverActive(bool bActive);
	void ActivateSkillNodeButtonByGamepad();

	// Setter
	UFUNCTION(BlueprintCallable)
	void SetSkillNodeState(const ESkillNodeState InSkillNodeState);
	
	// Delegate
	UPROPERTY(BlueprintAssignable, Category = SkillTree)
	FOnSkillNodeClickedDelegate OnSkillNodeClicked;
	
protected:
	// 재정의 함수
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> SkillNodeButton;
	
// 데이터
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = "true"), Category = SkillTree)
	int32 SkillId = -1;
	
	// 게임 중 스킬 상태
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SkillTree, FieldNotify)
	ESkillNodeState SkillNodeState = ESkillNodeState::Locked;
	
private:
	// 브로드캐스팅 -> SkillTree에서 수신
	UFUNCTION()
	void HandleSkillNodeButtonClicked();

	void FinishGamepadButtonClick();
	
protected:
	// 상태 변경 시 블루프린트에서 값 수정
	UFUNCTION(BlueprintImplementableEvent)
	void OnSkillNodeStateChanged();

	// 마우스 올린 경우 실행
	UFUNCTION()
	void HandleSkillNodeButtonHovered();

	// 마우스 뗀 경우 실행
	UFUNCTION()
	void HandleSkillNodeButtonUnhovered();

	bool bGamepadHoverActive = false;
	bool bGamepadClickPending = false;
	FTimerHandle GamepadClickTimerHandle;
};
