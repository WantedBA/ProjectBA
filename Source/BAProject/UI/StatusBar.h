// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatusBar.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UStatusBar : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* Bar; // StatusBar 연결용

	// 최종 도달해야 할 목표 퍼센트
	float TargetPercent = 1.0f;

	// 화면에 보여지고 있는 퍼센트
	float CurrentDisplayPercent = 1.0f;

	// 증감 속도
	UPROPERTY(EditAnywhere, Category = "BA|UI|Settings")
	float InterpSpeed = 5.0f;

public:
	// Bar 퍼센트 조절
	void SetProgress(float Current, float Max);

	// 매 프레임 수치 보간하기 위한 Tick
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
