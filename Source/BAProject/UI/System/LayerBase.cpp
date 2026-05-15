// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/LayerBase.h"

void ULayerBase::OnPushed()
{
	// 로그: 스택에 들어옴 체크
	// GetName() 으로 어떤 위젯인지 확인)
	UE_LOG(LogTemp, Log, TEXT("[%s] OnPushed: UI가 스택에 추가됨"), *GetName());

	// 화면에 출력되도록 설정
	SetVisibility(ESlateVisibility::Visible);
}

void ULayerBase::OnPopped()
{
	// 로그: 스택에서 제거됨 체크
	// GetName() 으로 어떤 위젯인지 확인
	UE_LOG(LogTemp, Log, TEXT("[%s] OnPushed: UI가 스택에서 제거됨"), *GetName());

	// 화면에서 숨김처리 되도록 설정
	SetVisibility(ESlateVisibility::Collapsed);
}
