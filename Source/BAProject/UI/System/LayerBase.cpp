// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/System/LayerBase.h"

void ULayerBase::StartCloseProcess()
{
	// 블루프린트에서 애니메이션이 연결된 경우
	if (OutAnimation)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] StartCloseProcess: 애니메이션 재생 시작"), *GetName());

		PlayAnimation(OutAnimation);

		// 애니메이션이 끝나면 실행될 Bind
		FWidgetAnimationDynamicEvent EndEvent;
		EndEvent.BindDynamic(this, &ULayerBase::OnOutAnimationFinished);
		BindToAnimationFinished(OutAnimation, EndEvent);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] StartCloseProcess: 애니메이션 없음(즉시 종료)"), *GetName());

		OnOutAnimationFinished();
	}
}

void ULayerBase::OnOutAnimationFinished()
{
	if (OnCloseAnimationFinished.IsBound())
	{
		OnCloseAnimationFinished.Broadcast(this);
	}
}

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

}
