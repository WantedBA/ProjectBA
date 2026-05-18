// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/System/LayerBase.h"
#include "MainHUD.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UMainHUD : public ULayerBase
{
	GENERATED_BODY()

protected:
	// PlayerStatus 연결
	UPROPERTY(meta = (BindWidget))
	class UPlayerStatus* PlayerStatus; // HUD 상태창 연결

	// 미니맵 위젯 연결
	UPROPERTY(meta = (BindWidget))
	class UMinimapLayer* Minimap;

	// HP 물약 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BA|UI")
	class UQuickSlotBase* HPSlot;

	// 스테미너 물약 퀵슬롯 위젯
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BA|UI")
	class UQuickSlotBase* StaminaSlot;

	// 보스 체력바 위젯(WBP 이름 맞출 것)
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "BA|UI")
	class UBossHPBar* BossHPBar;

public:
	// 값 업데이트
	void UpdatePlayerHP(float Current, float Max);

	// 보스 데이터 업데이트 및 표시
	UFUNCTION(BlueprintCallable, Category = "BA|UI|Boss")
	void UpdateBossStatus(FText Name, float Current, float Max);
	
};
