// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/BALayerBase.h"

// 디버깅 로그 매크로
DEFINE_LOG_CATEGORY_STATIC(LogBALayer, Log, All);

void UBALayerBase::OnPushed()
{
	// 로그: 스택에 들어옴 체크
	// GetName() 으로 어떤 위젯인지 확인)
	UE_LOG(LogBALayer, Log, TEXT("[%s] OnPushed: UI가 스택에 추가됨"), *GetName());

	// 화면에 출력되도록 설정
	SetVisibility(ESlateVisibility::Visible);
}

void UBALayerBase::OnPopped()
{
	// 로그: 스택에서 제거됨 체크
	// GetName() 으로 어떤 위젯인지 확인
	UE_LOG(LogBALayer, Log, TEXT("[%s] OnPushed: UI가 스택에서 제거됨"), *GetName());

	// 화면에서 숨김처리 되도록 설정
	SetVisibility(ESlateVisibility::Collapsed);
}
