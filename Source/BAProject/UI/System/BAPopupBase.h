// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/System/BALayerBase.h"
#include "BAPopupBase.generated.h"

/**
 * 
 */
UCLASS()
class BAPROJECT_API UBAPopupBase : public UBALayerBase
{
	GENERATED_BODY()

public:
	UBAPopupBase(const FObjectInitializer& ObjectInitializer);

	// 인터페이스 함수 재정의
	// Popup 타입인지 반환
	virtual EStackElemType GetStackType() const override { return EStackElemType::Popup; }

	// 팝업창 닫기버튼 클릭 시 호출될 함수
	UFUNCTION(BlueprintCallable, Category = "UI")
	virtual void ClosePopup();
	
};
