// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatus.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UPlayerStatus : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 위젯이 생성될 때 초기화 담당 함수
	virtual void NativeConstruct() override;

	// HP바 연결
	UPROPERTY(meta = (BindWidget))
	class UStatusBar* HPBar; 
	// 스테미너바 연결
	UPROPERTY(meta = (BindWidget))
	class UStatusBar* StaminaBar; 

public:
	// 델리게이트에서 수신한 값을 실제 UI 컴포넌트에 반영
	UFUNCTION()
	void UpdateHP(float Current, float Max);
	UFUNCTION()
	void UpdateStamina(float Current, float Max);
};
