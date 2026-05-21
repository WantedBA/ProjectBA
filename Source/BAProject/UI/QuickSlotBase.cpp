// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/QuickSlotBase.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/OverlaySlot.h"


void UQuickSlotBase::NativeConstruct()
{
	Super::NativeConstruct();

	// SlotIcon 위젯에서 머티리얼을 가져와 실시간 파라미터 수정이 가능한 다이나믹 머티리얼로 변환
	if (SlotIcon)
	{
		IconMID = SlotIcon->GetDynamicMaterial();
	}
}

void UQuickSlotBase::NativeTick(const FGeometry& MyGeomtry, float InDeltaTime)
{
	Super::NativeTick(MyGeomtry, InDeltaTime);

	// 쿨타임이 활성화된 상태라면 매 프레임 남은 시간을 깎고 머티리얼 마스크를 조절
	if (bIsCooldownActive && IconMID)
	{
		CurrnetCooldown -= InDeltaTime;

		// 쿨타임 진행 비율 계산 (1.0 -> 0.0)
		float Percent =1.0f - FMath::Clamp(CurrnetCooldown / MaxCooldown, 0.0f, 1.0f);

		// 머티리얼 그래프에서 만든 'CooldownPercent' 파라미터에 값을 주입
		IconMID->SetScalarParameterValue(TEXT("CooldownPercent"), Percent);

		// 쿨타임 종료 판정
		if (CurrnetCooldown <= 0.0f)
		{
			bIsCooldownActive = false;

			IconMID->SetScalarParameterValue(TEXT("CooldownPercent"), 1.0f);
		}
	}
}

void UQuickSlotBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (SlotIcon && !IconMID)
	{
		IconMID = SlotIcon->GetDynamicMaterial();
	}

	// 에디터에서 이미지 적용 시 디자이너 창에 즉각 반영
	if (DefaultIcon)
	{
		SetSlotIcon(DefaultIcon);
	}

	if (HotKeyText)
	{
		HotKeyText->SetText(HotKeyName);
	}

	// 배경 및 테두리 PNG 적용
	if (SlotBG && BGTexture)
	{
		SlotBG->SetBrushFromTexture(BGTexture);
	}
	if (SlotFrame && FrameTexture)
	{
		SlotFrame->SetBrushFromTexture(FrameTexture);
	}

	// 아이콘 패딩 적용
	if (SlotIcon)
	{
		UOverlaySlot* IconSlot = Cast<UOverlaySlot>(SlotIcon->Slot);
		if (IconSlot)
		{
			IconSlot->SetPadding(IconPadding);
		}
	}
}

void UQuickSlotBase::StartCooldown(float Duration)
{
	// 쿨타임 지속 시간을 설정하고 타이머를 시작함
	if (Duration <= 0.0f)
	{
		return;
	}

	MaxCooldown = Duration;
	CurrnetCooldown = Duration;
	bIsCooldownActive = true;

	// 쿨타임이 시작되면 슬롯을 어둡게 만듦
	if (IconMID)
	{
		IconMID->SetScalarParameterValue(TEXT("CooldownPercent"), 0.0f);
	}
}


void UQuickSlotBase::UpdateCount(int32 NewCount)
{
	// 개수 텍스트를 화면에 표시
	if (CountText)
	{
		CountText->SetText(FText::AsNumber(NewCount));
	}

	// 물약이 0개라면 'bIsLocked' 파라미터를 1.0으로 만들어 아이콘을 흑백으로 변경함
	if (IconMID)
	{
		float LockedVal = (NewCount <= 0) ? 1.0f : 0.0f;
		IconMID->SetScalarParameterValue(TEXT("bIsLocked"), LockedVal);
	}
}

void UQuickSlotBase::SetSlotIcon(UTexture2D* NewIcon)
{
	if (NewIcon && IconMID)
	{
		// 머티리얼 안에 있는 'IconTexture' 이름의 칸에 새로운 이미지 적용
		IconMID->SetTextureParameterValue(TEXT("IconTexture"), NewIcon);
	}
}


	

	