// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SkillTree/SkillNodeWidget.h"

#include "Components/Button.h"
#include "Instance/SkillTreeSubsystem.h"
#include "UI/System/SubSystemUI.h"
#include "UI/MainHUD.h"
#include "Engine/GameInstance.h"


void USkillNodeWidget::SetSkillNodeState(const ESkillNodeState InSkillNodeState)
{
	SkillNodeState = InSkillNodeState;
	
	// UI 갱신
	OnSkillNodeStateChanged();
	
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

void USkillNodeWidget::HandleSkillNodeButtonClicked()
{
	OnSkillNodeClicked.Broadcast(SkillId);
}

void USkillNodeWidget::SetGamepadHoverActive(const bool bActive)
{
	if (!SkillNodeButton || bGamepadHoverActive == bActive)
	{
		return;
	}

	bGamepadHoverActive = bActive;
	if (bGamepadHoverActive)
	{
		SkillNodeButton->OnHovered.Broadcast();
	}
	else
	{
		SkillNodeButton->OnUnhovered.Broadcast();
	}
}

void USkillNodeWidget::ActivateSkillNodeButtonByGamepad()
{
	if (!SkillNodeButton || bGamepadClickPending)
	{
		return;
	}

	bGamepadClickPending = true;
	SkillNodeButton->OnPressed.Broadcast();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GamepadClickTimerHandle,
			this,
			&USkillNodeWidget::FinishGamepadButtonClick,
			0.05f,
			false);
		return;
	}

	FinishGamepadButtonClick();
}

void USkillNodeWidget::FinishGamepadButtonClick()
{
	if (!SkillNodeButton)
	{
		bGamepadClickPending = false;
		return;
	}

	SkillNodeButton->OnReleased.Broadcast();
	SkillNodeButton->OnClicked.Broadcast();
	bGamepadClickPending = false;
}

void USkillNodeWidget::HandleSkillNodeButtonHovered()
{
	UE_LOG(LogTemp, Warning, TEXT("!!! SkillNode Hovered !!! ID: %d"), SkillId);

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

void USkillNodeWidget::HandleSkillNodeButtonUnhovered()
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
