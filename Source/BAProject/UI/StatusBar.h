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

public:
	UStatusBar(const FObjectInitializer& ObjectInitializer);
	
protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* Bar; // StatusBar 연결용

	UPROPERTY(meta = (BindWidget))
	class UImage* BGImage;

	UPROPERTY(meta = (BindWidget))
	class UImage* FrameImage;

	// 설계 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BA|UI")
	class UMaterialInterface* BarMaterial;

	// 게이지 내부 PNG
	UPROPERTY(EditAnywhere, Category = "BA|UI")
	class UTexture2D* GaugeTexture;

	// 배경 PNG
	UPROPERTY(EditAnywhere, Category = "BA|UI")
	class UTexture2D* BGTexture;

	// 테두리 PNG
	UPROPERTY(EditAnywhere, Category = "BA|UI")
	class UTexture2D* FrameTexture;

	// 게이지 위치 조절 패딩
	UPROPERTY(EditAnywhere, Category = "BA|UI")
	FMargin BarPadding;

	// 실시간 수치 조절용 머티리얼 인스턴스
	UPROPERTY()
	class UMaterialInstanceDynamic* BarMID;

	// Bar 업데이트 여부 체크
	bool bIsFirstUpdeta = true;

	// 최종 도달해야 할 목표 퍼센트
	float TargetPercent = 1.0f;

	// 화면에 보여지고 있는 퍼센트
	float CurrentDisplayPercent = 1.0f;

	// 위젯 생성 시 초기화를 위한 함수
	virtual void NativeConstruct() override;

	// 증감 속도
	UPROPERTY(EditAnywhere, Category = "BA|UI|Settings")
	float InterpSpeed = 5.0f;


public:
	// Bar 퍼센트 조절
	void SetProgress(float Current, float Max);

	// 매 프레임 수치 보간하기 위한 Tick
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;


protected:
	// 에디터 및 게임 시작 시 머티리얼 적용 함수
	virtual void NativePreConstruct() override;
};
