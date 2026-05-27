// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SkillTree/SkillNodeWidget.h"

#include "Components/Button.h"
#include "Instance/SkillTreeSubsystem.h"
#include "UI/System/SubSystemUI.h"
#include "UI/MainHUD.h"
#include "Engine/GameInstance.h"


void USkillNodeWidget::SetSkillNodeState(const ESkillNodeState InSkillNodeState)
{
	if (SkillNodeButton && bSelectionFeedbackActive && bHasDefaultButtonStyle)
	{
		SkillNodeButton->SetStyle(DefaultButtonStyle);
	}

	SkillNodeState = InSkillNodeState;
	
	// UI 갱신
	OnSkillNodeStateChanged();
	CacheDefaultButtonStyle();
	RefreshSkillNodeSelectionFeedback();
	
	// TODO: 디버그용 임시 코드 
	FString StateString = StaticEnum<ESkillNodeState>()->GetNameStringByValue(static_cast<int64>(SkillNodeState));
	UE_LOG(LogTemp, Log, TEXT("Skill node %d state changed to %s"), SkillId, *StateString);
}

void USkillNodeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	// 앵커 중앙으로 설정
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

void USkillNodeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (SkillNodeButton)
	{
		CacheDefaultButtonStyle();

		SkillNodeButton->OnClicked.AddDynamic(this, &USkillNodeWidget::HandleSkillNodeButtonClicked);

		// 마우스 올린 경우 및 뗐을 때 이벤트 연결
		SkillNodeButton->OnHovered.AddDynamic(this, &USkillNodeWidget::HandleSkillNodeButtonHovered);
		SkillNodeButton->OnUnhovered.AddDynamic(this, &USkillNodeWidget::HandleSkillNodeButtonUnhovered);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SkillNodeButton is not initialized"));
	}
}

void USkillNodeWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);

	bNavigationFocused = true;
	RefreshSkillNodeSelectionFeedback();
}

void USkillNodeWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);

	bNavigationFocused = false;
	RefreshSkillNodeSelectionFeedback();
}

void USkillNodeWidget::HandleSkillNodeButtonClicked()
{
	OnSkillNodeClicked.Broadcast(SkillId);
}

void USkillNodeWidget::HandleSkillNodeButtonHovered()
{
	bPointerHovered = true;
	RefreshSkillNodeSelectionFeedback();
}

void USkillNodeWidget::HandleSkillNodeButtonUnhovered()
{
	bPointerHovered = false;
	RefreshSkillNodeSelectionFeedback();
}

void USkillNodeWidget::RefreshSkillNodeSelectionFeedback()
{
	const bool bShouldShowSelectionFeedback = bPointerHovered || bNavigationFocused;
	const bool bWasSelectionFeedbackActive = bSelectionFeedbackActive;
	bSelectionFeedbackActive = bShouldShowSelectionFeedback;

	ApplySkillNodeButtonSelectionStyle(bSelectionFeedbackActive);

	if (!bWasSelectionFeedbackActive && bSelectionFeedbackActive)
	{
		ShowSkillNodeTooltip();
	}
	else if (bWasSelectionFeedbackActive && !bSelectionFeedbackActive)
	{
		HideSkillNodeTooltip();
	}
}

void USkillNodeWidget::CacheDefaultButtonStyle()
{
	if (!SkillNodeButton)
	{
		bHasDefaultButtonStyle = false;
		return;
	}

	DefaultButtonStyle = SkillNodeButton->GetStyle();
	bHasDefaultButtonStyle = true;
}

void USkillNodeWidget::ApplySkillNodeButtonSelectionStyle(const bool bSelected)
{
	if (!SkillNodeButton || !bHasDefaultButtonStyle)
	{
		return;
	}

	if (!bSelected)
	{
		SkillNodeButton->SetStyle(DefaultButtonStyle);
		return;
	}

	FButtonStyle SelectedStyle = DefaultButtonStyle;
	SelectedStyle.Normal = DefaultButtonStyle.Hovered;
	SelectedStyle.NormalForeground = DefaultButtonStyle.HoveredForeground;
	SkillNodeButton->SetStyle(SelectedStyle);
}

void USkillNodeWidget::ShowSkillNodeTooltip()
{
	UE_LOG(LogTemp, Warning, TEXT("!!! SkillNode Selected !!! ID: %d"), SkillId);

	// GameInstance 가져오기
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	// GameInstance 에서 UI 찾기
	USubSystemUI* UISubsystem = GI->GetSubsystem<USubSystemUI>();
	if (!UISubsystem)
	{
		return;
	}

	// 툴팁 요청
	if (UMainHUD* MainHUD = UISubsystem->GetMainHUD())
	{
		MainHUD->RequestShowTooltip(SkillId);
	}
}

void USkillNodeWidget::HideSkillNodeTooltip()
{
	// SubSystemUI 찾기
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USubSystemUI* UISubsystem = GI->GetSubsystem<USubSystemUI>())
		{
			// HUD를 찾아서 툴팁 숨기기
			if (UMainHUD* MainHUD = UISubsystem->GetMainHUD())
			{
				MainHUD->HideTooltip();
			}
		}
	}
}
