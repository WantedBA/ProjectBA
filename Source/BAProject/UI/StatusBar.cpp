// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/StatusBar.h"
#include "Components\ProgressBar.h"

void UStatusBar::SetProgress(float Current, float Max)
{
	// 예외 체크
	if (Bar)
	{
		// ProgressBar 퍼센트 계산
		float Percent = (Max > 0.f) ? (Current / Max) : 0.f;

		// 위젯에 값 업데이트
		Bar->SetPercent(Percent);
	}
}
