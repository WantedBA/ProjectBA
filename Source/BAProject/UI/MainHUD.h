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

public:
	// 값 업데이트
	void UpdatePlayerHP(float Current, float Max);
	
};
