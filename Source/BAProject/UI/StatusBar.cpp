// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/StatusBar.h"
#include "Components\ProgressBar.h"

UStatusBar::UStatusBar(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// 보간 속도 기본값 설정(에디터에서 변경 가능)
	InterpSpeed = 5.0f;
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
		CurrentDisplayPercent = FMath::FInterpTo(CurrentDisplayPercent, TargetPercent, InDeltaTime, InterpSpeed);
		Bar->SetPercent(CurrentDisplayPercent);
	}
}
