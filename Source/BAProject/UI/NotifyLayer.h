// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/System/LayerBase.h"
#include "NotifyLayer.generated.h"

/**
 * 모든 알림 총괄하는 최상위 레이어
 */
UCLASS()
class BAPROJECT_API UNotifyLayer : public ULayerBase
{
	GENERATED_BODY()

public:
	UNotifyLayer(const FObjectInitializer& ObjectInitializer);

	// FadeIn, Out 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Notify")
	void PlayFadeEffect(bool bFadeIn);

	// 데이터 테이블 ID
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Notify")
	void ShowSplashMessageByTid(int32 Tid);

	// 상호작용 안내 ON,OFF 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Notify")
	void SetInteractionNotice(bool bShow, int32 Tid = 0);

	// 애니메이션 종료 감지 함수
	UFUNCTION()
	void OnFadeInAnimationFinished();

	// 상호작용 안내 문구 제어 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Notify")
	void SetInteractionText(bool bShow, FText CustomText = FText::GetEmpty());

protected:
	virtual void NativeConstruct() override;

protected:
	// Fade용 위젯
	UPROPERTY(meta = (BindWidget))
	class UImage* FadeImage;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* FadeInAnim;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* FadeOutAnim;

	// 지역 이름 등 중앙에 표시 될 텍스트
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* SplashText;

	// 상호작용 안내 텍스트
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* InteractionText;

	// 텍스트 전용 애니메이션
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* SplashAnim;
	
};
