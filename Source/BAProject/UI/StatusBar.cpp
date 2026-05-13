// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/StatusBar.h"
#include "Components\ProgressBar.h"

void UStatusBar::SetProgress(float Current, float Max)
{
	// 즉시 바를 깎는 대신, 도달해야 할 목표 설정
	TargetPercent = (Max > 0.f) ? (Current / Max) : 0.f;
}

void UStatusBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 현재 보여지는 값이 목표값과 다른지 체크
	if (!FMath::IsNearlyEqual(CurrentDisplayPercent, TargetPercent, 0.001f))
	{
		// FInterpTo: 현재값에서 목표값으로 부드럽게 이동
		// 델타 타임을 사용하여 프레임이 끊겨도 일정한 속도로 움직이게 함
		CurrentDisplayPercent = FMath::FInterpTo(CurrentDisplayPercent, TargetPercent, InDeltaTime, InterpSpeed);

		// 보간 값을 실제 위젯에 반영
		if (Bar)
		{
			Bar->SetPercent(CurrentDisplayPercent);
		}
	}
}
