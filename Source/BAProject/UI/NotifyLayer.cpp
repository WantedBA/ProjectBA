// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NotifyLayer.h"
#include "Components/Image.h"
#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"

#include "Tables/BATableManager.h"
#include "Tables/Text.h"


UNotifyLayer::UNotifyLayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SortOrder = 10;
}

void UNotifyLayer::PlayFadeEffect(bool bFadeIn)
{
	if (bFadeIn && FadeInAnim)
	{
		FadeImage->SetVisibility(ESlateVisibility::Visible);

		// 애니메이션 종료 후 지우기
		FWidgetAnimationDynamicEvent EndEvent;
		EndEvent.BindDynamic(this, &UNotifyLayer::OnFadeInAnimationFinished);
		BindToAnimationFinished(FadeInAnim, EndEvent);

		PlayAnimation(FadeInAnim);
	}
	else if(!bFadeIn && FadeOutAnim)
	{
		FadeImage->SetVisibility(ESlateVisibility::Visible);
		PlayAnimation(FadeOutAnim);
	}
}

void UNotifyLayer::ShowSplashMessageByTid(int32 Tid)
{
	const FTextRows* TextRow = UBATableManager::Get(this)->FindText(Tid);
	
	if (TextRow && SplashText && SplashAnim)
	{
		SplashText->SetText(FText::FromString(TextRow->KoreanText));

		StopAnimation(SplashAnim);
		SplashText->SetRenderOpacity(0.0f);

		PlayAnimation(SplashAnim);
	}
}

void UNotifyLayer::SetInteractionNotice(bool bShow, int32 Tid)
{
	if (!InteractionText)
	{
		return;
	}

	if (bShow)
	{
		// 테이블에서 텍스트 찾기
		if (const FTextRows* TextRow = UBATableManager::Get(this)->FindText(Tid))
		{
			InteractionText->SetText(FText::FromString(TextRow->KoreanText));
			InteractionText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// 텍스트 숨기기
			InteractionText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UNotifyLayer::OnFadeInAnimationFinished()
{
	if (FadeImage)
	{
		FadeImage->SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogTemp, Warning, TEXT(">>> NotifyLayer: Fade In Finished. Path Cleared."));
	}
}

void UNotifyLayer::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (FadeImage)
	{
		FadeImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}