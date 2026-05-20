// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseWidget.h"
#include "QuickSlotBase.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UQuickSlotBase : public UBaseWidget
{
	GENERATED_BODY()
	
protected:
	// 슬롯에 표시될 아이콘 이미지 (WBP에서 이름 맞춰줄 것)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "BA|UI")
	class UImage* SlotIcon;

	// 남은 아이템 개수 표시할 텍스트(WBP에서 이름 맞춰줄 것)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"), Category = "BA|UI")
	class UTextBlock* CountText;

	// 실시간 쿨타임 및 흑백 효과 제어를 위한 다이나믹 머티리얼
	UPROPERTY()
	class UMaterialInstanceDynamic* IconMID;

	// 쿨타임 관리용 내부 변수
	float MaxCooldown = 0.0f;
	float CurrnetCooldown = 0.0f;
	bool bIsCooldownActive = false;

	// 에디터 패널에서 슬롯별 아이콘 적용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BA|UI")
	class UTexture2D* DefaultIcon;

	// 퀵슬롯에 표시될 단축키 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BA|UI")
	FText HotKeyName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* HotKeyText;

	// 테두리 이미지 위젯 바인딩
	UPROPERTY(meta = (BindWodget))
	class UImage* SlotFrame;

	// 배경 이미지 위젯 바인딩
	UPROPERTY(meta = (BindWodget))
	class UImage* SlotBG;

	UPROPERTY(EditAnywhere, Category = "BA|UI")
	class UTexture2D* FrameTexture;

	UPROPERTY(EditAnywhere, Category = "BA|UI")
	class UTexture2D* BGTexture;

	// 패딩
	UPROPERTY(EditAnywhere, Category = "BA|UI")
	FMargin IconPadding;

public:
	// 아이템 사용 시 호출하여 쿨타임을 시작하는 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI|QuickSlot")
	void StartCooldown(float Duration);

	UFUNCTION(BlueprintCallable, Category = "BA|UI|QuickSlot")
	void UpdateCount(int32 NewCount);

	// 에디터에서 슬롯의 아이콘 이미지를 설정하는 함수
	UFUNCTION(BlueprintCallable, Category = "BA|UI|QuickSlot")
	void SetSlotIcon(UTexture2D* NewIcon);

protected:
	// 위젯 초기화 시 머티리얼 인스턴스 생성
	virtual void NativeConstruct() override;

	// 매 프레임 쿨타임 수치를 계산하여 머리티얼에 전달
	virtual void NativeTick(const FGeometry& MyGeomtry, float InDeltaTime) override;

	// 에디터 실시간 반영 함수
	virtual void NativePreConstruct() override;

};
