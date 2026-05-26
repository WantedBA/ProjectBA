// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/StatusBar.h"
#include "Components\ProgressBar.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"

UStatusBar::UStatusBar(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// 보간 속도 기본값 설정(에디터에서 변경 가능)
	InterpSpeed = 5.0f;
}

void UStatusBar::SetProgressImmediate(float Value)
{
	// 목표값과 현재 표시값을 모두 인자값으로 강제 고정
	TargetPercent = FMath::Clamp(Value, 0.0f, 1.0f);
	CurrentDisplayPercent = TargetPercent;

	if (Bar)
	{
		Bar->SetPercent(CurrentDisplayPercent);
	}
}

void UStatusBar::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯이 메모리에 올라오는 순간, 기본값을 100% 설정
	CurrentDisplayPercent = 1.0f;
	TargetPercent = 1.0f;
	bIsFirstUpdeta = false;

	if (Bar)
	{
		Bar->SetPercent(1.0f);
	}
}

void UStatusBar::SetProgress(float Current, float Max)
{
	// 즉시 바를 깎는 대신, 도달해야 할 목표 설정
	TargetPercent = (Max > 0.f) ? (Current / Max) : 0.f;
}

void UStatusBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 보간
	if (!bIsFirstUpdeta && Bar)
	{
		CurrentDisplayPercent = FMath::FInterpConstantTo(CurrentDisplayPercent, TargetPercent, InDeltaTime, InterpSpeed);

		if (FMath::IsNearlyEqual(CurrentDisplayPercent, TargetPercent, 0.001f))
		{
			CurrentDisplayPercent = TargetPercent;
		}

		// 일반 프로그레스바 수치 조절
		Bar->SetPercent(CurrentDisplayPercent);

		if (BarMID)
		{
			BarMID->GetScalarParameterValue(TEXT("Percent"), CurrentDisplayPercent);
		}
	}
}

void UStatusBar::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 에디터에서 BarMaterial 적용 시, 내부 프로그레스 바의 이미지를 교체
	if (Bar && BarMaterial)
	{
		// 머티리얼 인터페이스로부터 다이내믹 인스턴스 생성
		BarMID = UMaterialInstanceDynamic::Create(BarMaterial, this);

		// 머티리얼 내부의 파라미터에 적용
		if (GaugeTexture)
		{
			BarMID->SetTextureParameterValue(TEXT("BarTexture"), Cast<UTexture>(GaugeTexture));
		}

		// 배경 및 테두리 이미지 위젯 업데이트
		if (BGImage && BGTexture)
		{
			BGImage->SetBrushFromTexture(BGTexture);
		}
		if (FrameImage && FrameTexture)
		{
			FrameImage->SetBrushFromTexture(FrameTexture);
		}

		// 프로그레스 바 스타일 적용 
		// 프로그레스 바의 전체 스타일 구조체 복사
		FProgressBarStyle NewStyle = Bar->GetWidgetStyle();

		// 게이지 영역의 렌더링 리소스를 동적 머티리얼로 교체
		NewStyle.FillImage.SetResourceObject(BarMID);

		// 브러시의 그리기 방식을 이미지 형태로 보이도록 강제
		NewStyle.FillImage.DrawAs = ESlateBrushDrawType::Image;

		// 변경된 임시 스타일을 프로그레스 바에 최종 반영
		Bar->SetWidgetStyle(NewStyle);
	}

	// 패딩 적용
	if (Bar)
	{
		UOverlaySlot* BarSlot = Cast<UOverlaySlot>(Bar->Slot);
		if (BarSlot)
		{
			BarSlot->SetPadding(BarPadding);
		}
	}
	if (BGImage)
	{
		UOverlaySlot* BarSlot = Cast<UOverlaySlot>(BGImage->Slot);
		if (BarSlot)
		{
			BarSlot->SetPadding(BarPadding);
		}
	}
}
